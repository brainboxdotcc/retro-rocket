#include "../include/kernel.h"
#include "../include/kmalloc.h"
#include "../include/paging.h"
#include "../include/kprintf.h"

extern u32int end;		/* Heap straight after kernel */
u32int heap_pos = (u32int)&end;
//u32int heap_pos = 0x00200000;

u32int allocated = 0;
u32int allocations = 0;

extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

/* Two heaps, one for kernel and one for user */
heap_t*	kheap = 0;	/* Kernel Heap */
heap_t*	uheap = 0;	/* User Heap */

/* Our list will be ordered and will construct the internal structures */
header_t* ord_list_insert(header_t *insert,header_t *list);	/* Insert into ordered list */
header_t* ord_list_remove(header_t *remove,header_t *list);	/* Remove item from list, return list */
header_t* ord_list_get_nnode(u32int n,header_t *list);		/* Get N'th ordered node */
header_t* ord_list_get_last(header_t *list);			/* Get last ordered node */
header_t* ord_list_src_size(u32int sz,header_t *list);		/* We are looking for sz <BLOCK size */
header_t* ord_list_src_head(header_t* head,header_t *list);	/* Looking for node header */

/*Voithitikes routines tou memory manager */
static void* alloc(u32int size,u8int align,heap_t *heap);/* Allocate memory on the heap */
static void free_int(void*, heap_t *heap);		/* Free memory on the heap */
void expand_heap(u32int size, heap_t *heap);	/* Extend the heap by size bytes (paligned)*/
void shrink_heap(u32int size, heap_t *heap);	/* Shrink the heap to size bytes (paligned) */
void fix_heap_list(heap_t *heap);		/* Always ensure the block is within the heap */
header_t* palign_block(u32int size, heap_t *heap);	/* Page align block */

/********************************************************/

heap_t*	create_heap(u32int addr, u32int end, u32int max, u32int min, u8int user, u8int rw)
{
	heap_t* heap = kmalloc(sizeof(heap_t));
	header_t* header;
	footer_t* footer;

	assert((!(addr % 0x1000) && !(end % 0x1000)),"CREATE HEAP - NON PAGE ALIGNED");
	assert(addr < end, "CREATE HEAP - START > END");
	assert(max > end, "CREATE HEAP - END > MAX");

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
	footer = (footer_t*)((u32int)header + sizeof(header_t) + header->size);	/* seek */
	footer->magic = HEAP_MAGIC;		/* ID byte */
	footer->header = header;		/* point to header */

	return heap;
}

static void *alloc(u32int size,u8int palign, heap_t *heap)
{
	void *ret;		/* return address */
	u32int b_total, b_new;	/* Helpful for split */
	header_t *th;		/* pointer to block header */
	footer_t *tf;		/* pointer to block footer */

	if (!heap)
		return 0;	/* null heap */

	/* We are looking for a free block that fits the size */
	if (!ord_list_src_size(size, heap->list_free))	/* ... if there is no large enough free block */
		expand_heap(size - (ord_list_get_last(heap->list_free))->size, heap);	/* expand heap */
	if (palign)	/* If requested page aligned address */
		th = palign_block(size, heap);	/* Pass page aligned block */
	else		/* otherwise... */
		th = ord_list_src_size(size, heap->list_free);	/* Pass the first available block */

	if (!th)
		return 0;					/* We could not find space */

	heap->list_free = ord_list_remove(th, heap->list_free);	/* Remove the entry */
	tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);
	if (th->magic != HEAP_MAGIC)
		return 0;	/* Check if header */
	if (tf->magic != HEAP_MAGIC)
		return 0;	/* Check if footer */

	/* There are two possibilities, either it fits into the block, or split */
	b_total = sizeof(header_t) + th->size + sizeof(footer_t);	/* total size of the block */
	b_new	= sizeof(header_t) + size + sizeof(footer_t);		/* total size of the new entry */
	if (b_total - b_new > sizeof(header_t) +  sizeof(footer_t))
	{
		/* Fits in the empty block */
		/*  we split, copy the original block */
		th->free = 0;		/* Mark it used */
		th->size = size;	/* New size */
		tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;	/* New footer */
		tf->header = th;
		/* The address of the header to return */
		ret = ((void*)((u32int)th+sizeof(header_t)));

		/* Make aa new free block in the space */
		th = (header_t*)((u32int)tf + sizeof(footer_t));	/* New header for the free block */
		th->magic = HEAP_MAGIC;					/* ID byte */
		th->size = b_total - b_new - sizeof(header_t) - sizeof(footer_t);
		th->free = 1;						/* Make free */
		tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);	/* new footer */
		tf->magic = HEAP_MAGIC;	/* not necessary since there was already a footer */
		tf->header = th;	/* point to new header */

		/* Put a new entry for the new free block */
		heap->list_free = ord_list_insert(th,heap->list_free);
	}
	else 
	{
		/* It can be split */
		th->free = 0;		/* Mark as used */
		ret = ((void*)((u32int)th+sizeof(header_t)));
	}
	/* Finally, return the address in the header we created */
	return ret;
}

static void free_int(void *addr, heap_t *heap)
{
	header_t *th,*th_left, *th_right;	/* pointers to the current, previous and next block */
	footer_t *tf,*tf_left, *tf_right;	/* will use the prev and next to merge the list on deletion */

	if (!heap)
		return;	/* null heap */
	if (addr < (void*)heap->heap_addr || addr > (void*)heap->end_addr)
		return; /* addr not in heap */

	th = (header_t*)((u32int)addr - sizeof(header_t));		/* Pass the header address */
	tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);	/* and the footer */

	if (th->magic != HEAP_MAGIC)
		return;	/* Check if header */
	if (tf->magic != HEAP_MAGIC)
		return;	/* Check if footer */

	/* Find associated free blocks */
	/* Check if a free block left */
	if ((u32int)th != (u32int)heap->heap_addr)
	{
		/* if there is space left */
		tf_left = (footer_t*)((u32int)th - sizeof(footer_t));
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
	if ((u32int)tf + sizeof(footer_t) != (u32int)heap->end_addr)
	{
		/* ... if there is space to the right */
		th_right = (header_t*)((u32int)tf + sizeof(footer_t));
		if (th_right->magic == HEAP_MAGIC)
		{
			/* a free block... */
			tf_right = (footer_t*)((u32int)th_right + sizeof(header_t) + th_right->size);
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
	th->size = (u32int)tf - ((u32int)th + sizeof(header_t));
	tf->magic = HEAP_MAGIC;
	tf->header = th;

	if (heap->end_addr == (u32int)tf + sizeof(footer_t))
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
}

void expand_heap(u32int size, heap_t *heap)
{
	/* Expand to make the size of the heap a new aligned size */
	if (size & 0xFFF)
	{
		/* make it page aligned if it is not already */
		size &= 0xFFFFF000;
		size += 0x1000;
	}
	assert(heap != NULL, "HEAP EXPAND - NULL HEAP");
	/*expand up to max size */
	if (heap->end_addr + size > heap->max_size) 
		size = heap->max_size - heap->end_addr;
	/* We now define the pages */
	sign_sect(heap->end_addr, heap->end_addr+size, heap->user, heap->rw,current_directory);
	_memset((char*)heap->end_addr, 0 , size);	/* Nullify */
	heap->end_addr += size;				/* Adjust to end */
	fix_heap_list(heap);				/* Fix list */
}

void shrink_heap(u32int size, heap_t* heap)
{
	/* Shrink a heap, but not below min_size */
	u32int tmp = (u32int)heap->end_addr - size;
	tmp &= 0xFFFFF000;

	assert(heap != NULL, "HEAP SHRINK - NULL HEAP");
	if (tmp < heap->min_size)	/* Check for shrinking below min_size */
		tmp = heap->min_size;

	release_sect(tmp, heap->end_addr,current_directory);	/*Release pages */
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
	u8int *p;				/* seek byte */
	p = (u8int*)heap->end_addr - sizeof(footer_t);		/* We are looking for the 1-byte Magic Number */

	for (tf = (footer_t*)p; tf >= (footer_t*)heap->heap_addr; tf = (footer_t*)--p)
	{
		if (tf->magic == HEAP_MAGIC)
		{
			/* link check */
			if ((int)tf->header >= (int)heap->heap_addr &&
					(int)tf->header <= (int)heap->end_addr &&
					(int)tf->header->magic == HEAP_MAGIC)
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
		/*An den exei vrethei oute 1 block */
		/*Ftiaxnoume 1 plires free block, omoia me to heap_create*/
		th = (header_t*)(heap->heap_addr);
		th->magic = HEAP_MAGIC;
		th->size = (heap->end_addr-heap->heap_addr - sizeof(header_t) - sizeof(footer_t));
		th->free = 1;
		th->prev = 0;
		th->next = 0;
		/* define footer */
		tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;
		tf->header = th;

		heap->list_free = th;

		return;
	}
	else
	{
		/*an eimaste se pragmatiko block footer */
		th = tf->header;
		if (th->free)
		{
			/*An itan Free block, expand */
			heap->list_free = ord_list_remove(th,heap->list_free);	/*remove to palio entry */
			tf = (footer_t*)((u32int)heap->end_addr - sizeof(footer_t));	/*Footer sto telos tou heap */
			tf->magic = HEAP_MAGIC;
			tf->header = th;
			th->size = (u32int)tf - (u32int)th - sizeof(header_t);	/*fix size */
			heap->list_free = ord_list_insert(th,heap->list_free);	/*vazoume entry */
			return;
		}
		else
		{
			/*An den itan (itan used..) */
			th = (header_t*)((u32int)tf + sizeof(footer_t));		/*ftiaxnoume neo block amesos meta */

			th->magic = HEAP_MAGIC;
			th->free = 1;
			tf = (footer_t*)((u32int)heap->end_addr - sizeof(footer_t));	/*Footer sto telos tou heap */

			tf->magic = HEAP_MAGIC;					/*ftiaxnoume new footer */
			tf->header = th;
			th->size = (u32int)tf - (u32int)th - sizeof(header_t);	/*fix size */
			heap->list_free = ord_list_insert(th, heap->list_free);	/*vazoume entry */
			return;
		}
	}
}

header_t* palign_block(u32int size, heap_t *heap)
{
	header_t* th = ord_list_src_size(size, heap->list_free);	/*Penoume to 1o diathesimo block */
	header_t *tmp_h;
	footer_t *tmp_f, *tmp_f2;

	/*Ean kai efoson xreiazomaste Page Align, edw prepei na ginei */
	/*Ksekiname tin anazitisi enos Page aligned block, H dimiourgia enos */
	while(th)
	{
		if ( ((u32int)th + sizeof(header_t)) % 0x1000  == 0 )
			return th;	/*Page algned apo mono tou*/

		/*Efoson to block den einai paligned, koitame an mporoume na ftiaksoume emeis */
		tmp_f = (footer_t*)((u32int)th + sizeof(header_t) + th->size);	/*to footer tou block */
		tmp_h = th;
		tmp_h = (header_t*)((u32int)tmp_h & 0xFFFFF000);	/*pame to tmp_h se page bound */
		tmp_h = (header_t*)((u32int)tmp_h + 0x1000);		/*sto epomeno sigkekrimena */
		tmp_h = (header_t*)((u32int)tmp_h - sizeof(header_t));	/*To DATA address prepei na einai paligned, oxi to header */
		while ((u32int)tmp_h < ((u32int)tmp_f - sizeof(header_t)))
		{	/*to bound einai Prin apo to footer */
			if (((u32int)tmp_f - (u32int)tmp_h - sizeof(header_t) >= size ) &&/*Yparxei arketos xoros meta*/
			    ((u32int)tmp_h - (u32int)th > sizeof(header_t)+sizeof(footer_t)))/*Yparxei arketos xoros prin*/
			{
				/*Tote kanoume split to block se 2 free blocks kai kanoume alloc sto 2o */
				heap->list_free = ord_list_remove(th,heap->list_free);
				tmp_f2 = (footer_t*)((u32int)tmp_h - sizeof(footer_t));
				tmp_f2->magic = HEAP_MAGIC;
				tmp_f2->header = th;
				th->size = (u32int)tmp_f2 - (u32int)th - sizeof(header_t);
				heap->list_free = ord_list_insert(th,heap->list_free);
		
				th = tmp_h;
				th->magic = HEAP_MAGIC;
				th->size = (u32int)tmp_f - (u32int)th - sizeof(header_t);
				th->free = 1;
				tmp_f->header = th;
				tmp_f->magic = HEAP_MAGIC;
				heap->list_free = ord_list_insert(th,heap->list_free);
				return th;	/*eimaste etimoi */
			}
			tmp_h = (header_t*)((u32int)tmp_h + 0x1000);	/*sto epomeno  bound */
		}
		th = th->next;	/*den vrikame sosto bound mesa, epomeno block */
	}
	return th;	/*sto simeio afto to header pou dixnoume einai eite 0, eite page aligned */
}

void* kmalloc_org(u32int size, u8int align,u32int *phys)
{
	void* ret;

	if (align)
	{
		/*Morfopoioume se page boundaries */
		heap_pos &= 0xFFFFF000;
		heap_pos += 0x1000;	/*Next bound */
	}
	if (phys) /*epistrefoume to Alloc address sto phys */
		*phys = heap_pos;
	ret = (void*)heap_pos;
	heap_pos += size;
	return ret;
}

void* kmalloc_ext(u32int size, u8int align, u32int *phys)
{
	allocated += size;
	void* ret;
	if (kheap)
	{
		ret = alloc(size,align,kheap);
		if (phys)
		{	
			/*Vriskoume to physical address k to epistrefoume */
			page_t *page = get_page((u32int)ret, 0, current_directory);
			*phys = (page->frame * 0x1000) + (((u32int)ret) & 0xFFF);
		}
		return ret;
	}
	else
	{
		if (align)
		{
			/*Morfopoioume se page boundaries */
			heap_pos &= 0xFFFFF000;
			heap_pos += 0x1000;	/*Next bound */
		}
		if (phys) /*epistrefoume to Alloc address sto phys */
			*phys = heap_pos;
		ret = (void*)heap_pos;
		heap_pos += size;
		return ret;
	}
}

void* kmalloc(u32int size)
{	/*Pio genikis xriseos */
	return kmalloc_ext(size, 0, 0);
}

void kfree(void* addr)
{
	if (kheap)
		free_int(addr,kheap);
}

void* malloc_ext(u32int size, u8int align, u32int *phys)
{
	void* ret;
	if (uheap)
	{
		ret = alloc(size,align,uheap);
		if (phys)
		{	/*Vriskoume to physical address k to epistrefoume */
			page_t *page = get_page((u32int)ret, 0, current_directory);
			*phys = page->frame*0x1000 + (((u32int)ret) & 0xFFF);	/*+offset */
		}
		return ret;
	}
	return 0;
}

void* malloc(u32int size)
{	
	/*Pio genikis xriseos */
	return malloc_ext(size, 0, 0);
}

void free(void* addr)
{
	if (uheap)
		free_int(addr,uheap);
}

/*Ylopoiisi routinwn xeirismou taksinomimenis listas */
header_t* ord_list_insert(header_t *insert,header_t *list)
{
	header_t *tmp = list,*tmp2;
	if (!tmp)
	{
		/*Keni lista */
		insert->prev = 0;
		insert->next = 0;
		return insert;
	}
	if (tmp->size >= insert->size)
	{	
		/*Stin 1i thesi */
		tmp->prev = insert;
		insert->next = tmp;
		insert->prev = 0;
		return insert;
	}
	tmp2 = tmp;
	tmp = tmp->next;
	while (tmp)
	{
		/*Mesa stin lista */
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
	/*Sto telos tis listas */
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

header_t* ord_list_src_size(u32int sz,header_t *list)
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

header_t* ord_list_get_nnode(u32int n,header_t *list)
{
	header_t*tmp = list;
	u32int i;
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
		return 0;	/*Keni lista */
	while (tmp->next)
		tmp = tmp->next;
	return tmp;
}

