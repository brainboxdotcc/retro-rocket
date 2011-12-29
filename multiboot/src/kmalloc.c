#include "../include/kernel.h"
#include "../include/kmalloc.h"
#include "../include/paging.h"
#include "../include/kprintf.h"

extern u32int end;		/*Einai to telos tou telefteou section
				  h allios i arxi tou default heap mas ;) */
//u32int heap_pos = (u32int)&end;

// Heap starts at 1mb boundary, we have a higher-half kernel
// and most of the user tasks and programs are on the kernel heap
// as interpreted managed code, so we might as well start here and
// expand upwards.

u32int heap_pos = (u32int)0x200000;


extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
/*Tha yparxoun 2 energa heap, tou kernel kai tou proccess */
heap_t*	kheap = 0;	/*Kernel Heap */
heap_t*	uheap = 0;	/*User Heap */

/*H lista mas tha einai ordered kai tha ftiaksoume tis voithitikes routines xeirismou */
header_t* ord_list_insert(header_t *insert,header_t *list);	/*Insert se taksinomimeni lista */
header_t* ord_list_remove(header_t *remove,header_t *list);	/*Remove ap tin lista, return list */
header_t* ord_list_get_nnode(u32int n,header_t *list);		/*Epistrefei to N node */
header_t* ord_list_get_last(header_t *list);			/*Epistrefei Last node */
header_t* ord_list_src_size(u32int sz,header_t *list);		/*Psaxnoume gia sz < BLOCK size */
header_t* ord_list_src_head(header_t* head,header_t *list);	/*Psaxnei node me header */

/*Voithitikes routines tou memory manager */
static void* alloc(u32int size,u8int align,heap_t *heap);/*ALLOCATE memory apo heap */
static void free_int(void*, heap_t *heap);		/*FREE memory apo heap */
void expand_heap(u32int size, heap_t *heap);	/*Megalonoume to heap kata size bytes (paligned)*/
void shrink_heap(u32int size, heap_t *heap);	/*Mikrenoume to heap kata size bytes (paligned) */
void fix_heap_list(heap_t *heap);		/*Frontizei oste panta na yparxei block os to end */
header_t* palign_block(u32int size, heap_t *heap);

/********************************************************/

heap_t*	create_heap(u32int addr, u32int end, u32int max, u32int min, u8int user, u8int rw)
{
	heap_t* heap = kmalloc(sizeof(heap_t));
	header_t* header;
	footer_t* footer;

	assert((!(addr % 0x1000) && !(end % 0x1000)),"CREATE HEAP - NON PAGE ALIGNED");
	assert(addr < end, "CREATE HEAP - START > END");
	assert(max > end, "CREATE HEAP - END > MAX");

	_memset((char*)heap, 0, sizeof(heap_t));	/*Nullify */
 	heap->list_free = (header_t*)addr;	/*Sto 1o (monadiko) header */
	heap->heap_addr = addr;			/*Orizoume ta members tou heap */
	heap->end_addr = end;			/*heap end */
	heap->max_size = max;			/*MAX size */
	heap->min_size = min;			/*MIN size */
	heap->user = user;			/*Attributes gia ta pages sto expand */
	heap->rw = rw;				/*to idio */

	/*Ftiaxnoume to 1o block */
	header = (header_t*)addr;		/*Header, stin arxi tou heap */
	header->magic = HEAP_MAGIC;		/*ID byte */
	header->size = (end-addr - sizeof(header_t) - sizeof(footer_t));	/*size full */
	header->free = 1;			/*Diathesimo */
	header->prev = 0;			/*Proto entry (prev==0)*/
	header->next = 0;			/*Telefteo entry (next==0) */
	/*kai to footer */
	footer = (footer_t*)((u32int)header + sizeof(header_t) + header->size);	/*seek */
	footer->magic = HEAP_MAGIC;		/*ID byte */
	footer->header = header;		/*point sto header */

	return heap;				/*telos */
}

static void *alloc(u32int size,u8int palign, heap_t *heap)
{
	void *ret;		/*return address */
	u32int b_total, b_new;	/*Voithitika gia to split */
	header_t *th;		/*pointer gia block header */
	footer_t *tf;		/*pointer gia block footer */

	if (!heap)
		return 0;	/*null heap */

	/*Psaxnoume free block pou na xoraei to size */
	if (!ord_list_src_size(size, heap->list_free))	/*an den yparxei arketa megalo elefthero block */
		expand_heap(size - (ord_list_get_last(heap->list_free))->size, heap);	/*expand heap */
	if (palign)	/*An zitithike page aligned address */
		th = palign_block(size, heap);	/*Pernoume page aligned block */
	else		/*allios */
		th = ord_list_src_size(size, heap->list_free);	/*Penoume to 1o diathesimo block */

	if (!th)
		return 0;					/*Den vrethike xoros */

	heap->list_free = ord_list_remove(th, heap->list_free);	/*Afairoume to entry */
	tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);
	if (th->magic != HEAP_MAGIC)
		return 0;	/*Elegxos an uparxei header */
	if (tf->magic != HEAP_MAGIC)
		return 0;	/*Elegxos an uparxei footer */

	/*Yparxoun 2 periptoseis, H desmevoume olo to block, H to kanoume split */
	b_total = sizeof(header_t) + th->size + sizeof(footer_t);	/*Sinoliko size tou block */
	b_new	= sizeof(header_t) + size + sizeof(footer_t);		/*Sinoliko size tou neou */
	if (b_total - b_new > sizeof(header_t) +  sizeof(footer_t))
	{
		/*Xoraei block sto keno */
		/*Kanoume split, mikrenoume to arxiko block */
		th->free = 0;		/*Sign oti xrisimopoieitai */
		th->size = size;	/*Neo size */
		tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);
		tf->magic = HEAP_MAGIC;	/*Neo footer */
		tf->header = th;
		/*To address meta to header gia return */
		ret = ((void*)((u32int)th+sizeof(header_t)));

		/*Ftiaxnoume ena kainourio free block sto keno xoro */
		th = (header_t*)((u32int)tf + sizeof(footer_t));	/*Neo header gia free block */
		th->magic = HEAP_MAGIC;					/*ID byte */
		th->size = b_total - b_new - sizeof(header_t) - sizeof(footer_t);
		th->free = 1;						/*diathesimo */
		tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);	/*neo footer */
		tf->magic = HEAP_MAGIC;	/*den einai aparaitito kathos ipirxe footer idi */
		tf->header = th;	/*point sto neo header */

		/*Vazoume neo entry gia to kainourio free block */
		heap->list_free = ord_list_insert(th,heap->list_free);
	}
	else 
	{
		/*Den xoraei split opote to desmevoume olokliro */
		th->free = 0;		/*Sign oti xrisimopoieitai */
		ret = ((void*)((u32int)th+sizeof(header_t)));
	}
	/*Telos kanoume return tin diefthinsi meta to header pou desmefsame */
	return ret;
}

static void free_int(void *addr, heap_t *heap)
{
	header_t *th,*th_left, *th_right;	/*pointers gia current, previous kai next block */
	footer_t *tf,*tf_left, *tf_right;	/*tha xrisimopoiithoun ta prev kai next sto merge */

	if (!heap)
		return;	/*null heap */
	if (addr < (void*)heap->heap_addr || addr > (void*)heap->end_addr)
		return; /*addr not in heap */

	th = (header_t*)((u32int)addr - sizeof(header_t));		/*Pernoume to header */
	tf = (footer_t*)((u32int)th + sizeof(header_t) + th->size);	/*kai to footer */

	if (th->magic != HEAP_MAGIC)
		return;	/*Elegxos an uparxei header */
	if (tf->magic != HEAP_MAGIC)
		return;	/*Elegxos an uparxei footer */

	/*Enosi Free blocks */
	/*Tsekaroume an uparxei free block aristera */
	if ((u32int)th != (u32int)heap->heap_addr)
	{
		/*an yparxei xoros aristera */
		tf_left = (footer_t*)((u32int)th - sizeof(footer_t));
		if (tf_left->magic == HEAP_MAGIC && tf_left->header->magic == HEAP_MAGIC)
		{
			/*An yparxei block... */
			if (tf_left->header->free)
			{
				/*An einai free */
				th_left = tf_left->header;	/*Enosi aristera */
				heap->list_free = ord_list_remove(th_left,heap->list_free);
				th = th_left;
			}
		}
	}

	/*Tsekaroume an uparxei free block deksia */
	if ((u32int)tf + sizeof(footer_t) != (u32int)heap->end_addr)
	{
		/*an yparxei xoros deksia */
		th_right = (header_t*)((u32int)tf + sizeof(footer_t));
		if (th_right->magic == HEAP_MAGIC)
		{
			/*an uparxei block .. */
			tf_right = (footer_t*)((u32int)th_right + sizeof(header_t) + th_right->size);
			if (tf_right->magic == HEAP_MAGIC)
			{
				/*double check */
				if (th_right->free == 1)
				{
					/*an einai free */
					heap->list_free = ord_list_remove(th_right,heap->list_free);
					tf = tf_right;
				}
			}
		}
	}

	/*Opote pleon exoume ena free block me header == th kai footer == tf */
	th->free = 1;
	th->magic = HEAP_MAGIC;
	th->size = (u32int)tf - ((u32int)th + sizeof(header_t));
	tf->magic = HEAP_MAGIC;
	tf->header = th;

	if (heap->end_addr == (u32int)tf + sizeof(footer_t))
	{	
		/*An itan Last block */
		shrink_heap(th->size + sizeof(header_t) + sizeof(footer_t), heap);	/*Shrink */
	}
	else
	{	
		/*allios vazoume entry */
		heap->list_free = ord_list_insert(th,heap->list_free);
	}
}

void expand_heap(u32int size, heap_t *heap)
{
/*Kanei Expand to megethos tou heap kata size (paligned) bytes */
	/*To expand ginete se page boundaries */
	if (size & 0xFFF)
	{
		/*an den einai pagigned */
		size &= 0xFFFFF000;
		size += 0x1000;
	}
	assert(heap != NULL, "HEAP EXPAND - NULL HEAP");
	/*expand mexri max size */
	if (heap->end_addr + size > heap->max_size) 
		size = heap->max_size - heap->end_addr;
	/*Orizoume pages */
	sign_sect(heap->end_addr, heap->end_addr+size, heap->user, heap->rw,current_directory);
	_memset((char*)heap->end_addr, 0 , size);	/*Nullify */
	heap->end_addr += size;				/*Adjust to end */

	fix_heap_list(heap);				/*Fix list */
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
/*Frontizei etsi oste An kai efoson yparxei adesmeftos xoros sto telos tou heap
  na Orizete Block H na epekteinetai to yparxon. Mporei na xrisimopoieitai se kathe 
  Expand h akoma kai se Contract < Min_size */
	header_t* th;
	footer_t* tf;

	assert(heap != NULL, "HEAP FIX LIST - NULL HEAP");

	/*Adjust Blocks */
	/*Psaxnoume to telefteo footer sto heap */
	u8int *p;				/*seek byte */
	p = (u8int*)heap->end_addr - sizeof(footer_t);		/*Tha psaxnoume byte pros byte gia Magic Number */

	for (tf = (footer_t*)p; tf >= (footer_t*)heap->heap_addr; tf = (footer_t*)--p)
	{
		if (tf->magic == HEAP_MAGIC)
		{
			/*1o check */
			if (tf->header >= heap->heap_addr &&
					tf->header <= heap->end_addr &&
					tf->header->magic == HEAP_MAGIC)
			{
				/*Double check */
				goto bbreak;			/*vriskomaste se pragmatiko block end */
			}
		}
	}

bbreak:

	/*to tf mas vriskete eite sto last footer, eite sto start */
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
								/*to footer */
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

