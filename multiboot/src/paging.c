#include <kernel.h>
#include <io.h>
#include <interrupts.h>
#include <paging.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <debugger.h>
#include <multiboot.h>

extern u32int end;

page_directory_t *current_directory=0;	/* Currently active page directory */
page_directory_t *proc_initial = 0;	/* Initial page directory for user process */
page_directory_t *kernel_directory=0;	/* Kernel directory */

/* Representation of memory frames as a bitmap */
u32int *frames;
u32int nframes;

static u32int *l1,*l2,l3,l4;	/* Global variables for use in code where we may not have a safe stack */

MultiBoot* mb;

u32int heapstart, heaplen;

/* User and kernel memory heaps, defined in kmalloc.c */
extern u32int heap_pos;
extern u32int start;
extern heap_t*	kheap;
extern heap_t*	uheap;

/* Forward declarations */
static page_table_t *clone_table(page_table_t *src, u32int phys);
//static void kill_table(page_table_t *src);
static void copy_page_physical(u32int src, u32int dest);
static void set_frame(u32int frame_addr);
static void clear_frame(u32int frame_addr);
//static u32int test_frame(u32int frame_addr);
static u32int first_frame(void);
void alloc_frame(page_t *page, u8int f_usr, u8int f_rw);
void free_frame(page_t *page);

/* Page fault handler */
void page_fault_handler(registers_t* regs);

/* Initialise paging */
u32int init_paging(void* mbd)
{
	mb = (MultiBoot*)mbd;

	if (!(mb->flags & MB_MEMORY))
		preboot_fail("MultiBoot loader did not pass memory sizes to kernel.");

	u32int m_size = mb->mem_upper * 1024; 
	
	//nframes = m_size / 0x1000;			/* 0x1000 = page size */
	//frames = kmalloc(nframes / 32);			/* Allocate bitmap for pages */
	//_memset((char*)frames, 0, nframes / 32);	/* Set all contents to zero */

	//kernel_directory = kmalloc_ext(sizeof(page_directory_t), 1, 0);
	//_memset((char*)kernel_directory, 0, sizeof(page_directory_t));
	//kernel_directory->phys = (u32int)(&kernel_directory->tablesPhysical);
	//current_directory = kernel_directory;

	/* sign all sections for the kernel and its heap */
	//sign_sect(0, heap_pos + 0x100000, 0, 1, kernel_directory);	/*0 - heap_pos == KERNEL IMAGE,su,rw */

	register_interrupt_handler(14, page_fault_handler);	/* Install our page fault handler */
	//switch_page_directory(kernel_directory);		/* Enable paging by switching page directories */

	if (!(mb->flags & MB_MEMMAP))
		preboot_fail("MultiBoot loader did not pass BIOS memory map to kernel.");

	MB_MemMap* mm = mb->mmap_addr;
	u32int bestlen = 0;
	u32int bestaddr = 0;
	while ((u32int)mm < mb->mmap_len + mb->mmap_addr)
	{
		if (mm->lenlow > bestlen && mm->type == MB_MM_AVAIL)
		{
			bestaddr = mm->addrlow;
			bestlen = mm->lenlow;
		}
		if (mm->lenlow + mm->addrlow == 0x0) /* Wrapped around 32-bit value */
			break;
		else
			mm = (MB_MemMap*)((u32int)mm + mm->size + sizeof(mm->size));
	}

	// Nothing to speak of. less than 8mb free; give up!
	if (bestlen < 0x800000)
		preboot_fail("Less than 8mb of ram available, system halted.");

	heapstart = KHEAP_START;
	if (bestaddr > heapstart)	// Best block somehow above 4mb default heap pos
		heapstart = bestaddr;

	// Default heap to 128mb. If theres less ram than this in the machine, lower it.
	u32int min = 0x100000 * 128;
	if (bestlen < min)
		min = bestlen - 0x1000;

	heaplen = bestlen;

	//noisy_sign_sect(heapstart, heapstart + min, 0, 1, current_directory);
	kheap = create_heap(heapstart, min, bestlen - 0x1000, min, 0, 1);
	//current_directory = clone_directory(kernel_directory);
	//switch_page_directory(current_directory);

	return bestlen; 
}

void print_heapinfo()
{
	setforeground(current_console, COLOUR_LIGHTYELLOW);
	kprintf("HEAP: ");
	setforeground(current_console, COLOUR_WHITE);
	kprintf("Best fit; start=0x%08x max=0x%08x\n", heapstart, heaplen);
}

void sign_sect(u32int start, u32int end, u8int usr, u8int rw, page_directory_t *dir)
{
	return;
	u32int i;
	for (i = start; i < end; i+=0x1000)
		alloc_frame( get_page(i, 1, dir), usr, rw);
	/* Flush translation lookaside buffer */
	asm volatile("mov %%cr3, %0": "=r"(l3));	/* read cr3 */
	asm volatile("mov %0, %%cr3":: "r"(l3));	/* write cr3 */
}

void preboot_clrscr()
{
	u8int* clr = 0xB8000;
	for (; clr < 0xB8FFF; ++clr)
		*clr = 0;
}

void preboot_fail(char* msg)
{
	preboot_clrscr();
	u8int* screen = 0xB8000;
	for (; *msg; msg++)
	{
		*screen++ = *msg;
		*screen++ = 0x0F;
	}
	wait_forever();
}

void noisy_sign_sect(u32int start, u32int end, u8int usr, u8int rw, page_directory_t* dir)
{
	return;
	u32int i, m = 0;
	u8int* screenofs = 0xB8000;
	for (i = start; i < end; i+=0x1000)
	{
		alloc_frame( get_page(i, 1, dir), usr, rw);
		if (m++ > 0x500)
		{
			// Quick and dirty progress bar. No console driver this early in the boot sequence.
			*screenofs++ = '.';
			*screenofs++ = 0x0F;
			m = 0;
		}
	}
	/* Flush translation lookaside buffer */
	asm volatile("mov %%cr3, %0": "=r"(l3));
	asm volatile("mov %0, %%cr3":: "r"(l3));
}

void release_sect(u32int start, u32int end, page_directory_t *dir)
{
	u32int i;
	for (i = start; i < end; i+=0x1000)
		free_frame( get_page(i, 0, dir));
	/* Flush translation lookaside buffer */
	asm volatile("mov %%cr3, %0": "=r"(l3));	/*read cr0 */
	asm volatile("mov %0, %%cr3":: "r"(l3));	/*write cr0 */
}

void switch_page_directory(page_directory_t *dir)
{
	u32int cr0;
	current_directory = dir;
	asm volatile("mov %0, %%cr3":: "r"(dir->phys));
	asm volatile("mov %%cr0, %0": "=r"(cr0));	/* read cr0 */
	cr0 |= 0x80000000;				/* enable paging */
	asm volatile("mov %0, %%cr0":: "r"(cr0));	/* write cr0 */
}

page_t *get_page(u32int addr, u8int make, page_directory_t *dir)
{
	addr /= 0x1000;
	if (dir->tables[addr / 1024])
		return &dir->tables[addr / 1024]->pages[addr % 1024];
	else if (make)
	{
		u32int phys;
		dir->tables[addr / 1024] = kmalloc_ext(sizeof(page_table_t), 1, &phys);
		_memset((char*)dir->tables[addr / 1024], 0, sizeof(page_table_t));	/* we MUST zero this! */
		dir->tablesPhysical[addr / 1024] = phys | 0x7;	/* present, user, rw */
		return &dir->tables[addr / 1024]->pages[addr % 1024];
	}
	return 0;	/* We were asked to create a new page, but couldnt */
}

page_directory_t *clone_directory(page_directory_t *src)
{
	u32int phys,i;
	page_directory_t *ret;

	ret = (page_directory_t*)kmalloc_ext(sizeof(page_directory_t), 1, &phys);
	_memset((char*)ret, 0, sizeof(page_directory_t));
	ret->phys = phys + ((u32int)ret->tablesPhysical - (u32int)ret);

	for (i = 0; i < 1024; i++)
	{
		if (src->tables[i] == 0)
			continue;

		if (src->tables[i] == kernel_directory->tables[i]){
			ret->tables[i] = src->tables[i];
			ret->tablesPhysical[i] = src->tablesPhysical[i];
		}
		else {
			ret->tables[i] = clone_table(src->tables[i], (u32int)&phys);
			ret->tablesPhysical[i] = phys | 0x7;
		}
	}
	return ret;
}

void kill_directory(page_directory_t *src)
{
	u32int i;

	asm volatile("cli");
	if (src == current_directory)
		return;	/* Can't nuke the currently active page directory! */

	for (i = 0; i < 1024; i++)
	{
		if (src->tables[i] == 0)
			continue;

		if (src->tables[i] == kernel_directory->tables[i])
			continue;

//		FIXME: kill_table is buggy. We need to fix this.
//		Right now its choice between a memory leak and
//		a crash.
//
// 		kill_table(src->tables[i]);
//		kfree(src->tables[i]);
//		src->tables[i] = 0;
	}
	kfree(src);
	asm volatile("sti");
}

page_directory_t *init_procdir(void)
{
	page_directory_t *ret;
	ret = clone_directory(proc_initial);
	return ret;
}

static page_table_t *clone_table(page_table_t *src, u32int phys)
{
	u32int i;
	page_table_t *ret;

	ret = (page_table_t*)kmalloc_ext(sizeof(page_table_t), 1, (void*)phys);
	_memset((char*)ret, 0 , sizeof(page_table_t));

	for (i = 0; i < 1024; i++)
	{
		if (src->pages[i].frame == 0)
			continue;
		alloc_frame(&ret->pages[i], 0, 0);
		if (src->pages[i].present)
			ret->pages[i].present = 1;
		if (src->pages[i].rw)
			ret->pages[i].rw = 1;
		if (src->pages[i].user)
			ret->pages[i].user = 1;
		if (src->pages[i].accessed)
			ret->pages[i].accessed = 1;
		if (src->pages[i].dirty)
			ret->pages[i].dirty = 1;
		copy_page_physical(src->pages[i].frame * 0x1000, ret->pages[i].frame * 0x1000);
	}
	return ret;
}

/*static void kill_table(page_table_t *src)
{
	u32int i;

	for (i = 0; i < 1024; i++)
	{
		if (src->pages[i].frame == 0)
			continue;

		free_frame(&(src->pages[i]));
	}
}*/

static void copy_page_physical(u32int src, u32int dest)
{
	/* Paging must be DISABLED while we do this */
	l1 = (u32int*)src;
	l2 = (u32int*)dest;

	asm volatile("pushf");	/* Save Current EFLAGS - interrupt status */
	asm volatile("cli");	/* disable interrupts */

	/* Disable Paging */
	asm volatile("mov %%cr0, %0": "=r"(l3));
	l3 &= 0x7FFFFFFF;
	asm volatile("mov %0, %%cr0":: "r"(l3));

	for (l4 = 0; l4 < 1024; l4++)
		l2[l4] = l1[l4];

	/* Enable Paging */
	asm volatile("mov %%cr0, %0": "=r"(l3));
	l3 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "r"(l3));

	asm volatile("popf");	/*restore EFLAGS - restore interrupts */

}

static void set_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;	/* 0x1000 == page size */
	u32int index  = frame / 32;		/* 32 == sizeof(*frames) */
	u32int offset = frame % 32;
	frames[index] |= (0x1 << offset);
}

static void clear_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index  = frame / 32;
	u32int offset = frame % 32;
	frames[index] &= ~(0x1 << offset);
}

/*static u32int test_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index  = frame / 32;
	u32int offset = frame % 32;
	return (frames[index] & (0x1 << offset));
}*/

static u32int first_frame(void)
{
	u32int i = 0;
	u8int  tmp = 0;
	for (; i < nframes/32; i++)
	{
		if (frames[i] != 0xFFFFFFFF)
		{
			for (;tmp < 32; tmp++)
			{
				if (!(frames[i] & (0x1 << tmp)))
					return (i*32 + tmp);
			}
		}
	}
	kprintf("No free memory frames");
	return 0xFFFFFFFF;		/* Well, we're boned! */
}

/* Returns non-zero if a given physical address is unusable for page allocation */
int invalid_frame(u32int physaddr)
{
	//return 0;
	MB_MemMap* mm = mb->mmap_addr;
	while ((u32int)mm < mb->mmap_len + mb->mmap_addr)
	{
		if (physaddr >= mm->addrlow && physaddr < mm->addrlow + mm->lenlow - 1 && mm->type != MB_MM_AVAIL)
		{
			//kprintf("%08x falls within %08x + %08x\n", physaddr, mm->addrlow, mm->lenlow);
			return 1;
		}
		if (mm->lenlow + mm->addrlow == 0x0) /* Wrapped around 32-bit value */
			break;
		else
			mm = (MB_MemMap*)((u32int)mm + mm->size + sizeof(mm->size));
	}
	return 0;
}

void alloc_frame(page_t *page, u8int f_usr, u8int f_rw)
{
	if (page->frame == 0)
	{
		u32int index = first_frame();
		set_frame(index * 0x1000);
		page->frame = index;
		page->present = 1;
		page->user = (f_usr) ? 1 : 0;
		page->rw   = (f_rw) ? 1 : 0;
	}
}

void free_frame(page_t *page)
{
	if (page->frame)
	{
		clear_frame(page->frame);	/* free frame */
		page->frame = 0;
	}
}

void page_fault_handler(registers_t* regs)
{
	u32int faulting_address; 
	asm volatile("mov %%cr2, %0" : "=r" (faulting_address)); 

	PANIC_BANNER;
	kprintf("Page fault accessing address 0x%x, EIP=0x%x: ", faulting_address, regs->eip);
	int present = !(regs->err_code & 0x1);
	int rw = regs->err_code & 0x2;
	int us = regs->err_code & 0x4;
	int reserved = regs->err_code & 0x8;
	int id = regs->err_code & 0x10;

	kprintf("Page fault accessing address 0x%x, EIP=0x%x: %s%s%s%s (id=%d)", 
			faulting_address, 
			regs->eip, 
			present ? "present " : "", 
			rw ? "readonly " : "", 
			us ? "usermode " : "", 
			reserved ? "reserved " : "", 
			id); 
	backtrace(regs);
	blitconsole(current_console); 
	asm volatile("cli");
	wait_forever();
}

