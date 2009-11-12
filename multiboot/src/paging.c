#include "../include/kernel.h"
#include "../include/interrupts.h"
#include "../include/paging.h"
#include "../include/kmalloc.h"
#include "../include/printf.h"
#include "../include/io.h"

extern heap_t *kheap;

// The kernel's page directory
page_directory_t *kernel_directory = 0;

// The current page directory;
page_directory_t *current_directory = 0;

// A bitset of frames - used or free.
u32int *frames;
u32int nframes;

// Defined in kheap.c
extern u32int placement_address;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

static page_table_t *clone_table(page_table_t *src, u32int *physAddr);

extern void copy_page_physical(u32int src, u32int dest);

// Static function to set a bit in the frames bitset
static void set_frame(u32int frame_addr)
{
	u32int frame = frame_addr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(u32int frame_addr)
{
	u32int frame = frame_addr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static u32int UNUSED test_frame(u32int frame_addr)
{
	u32int frame = frame_addr/0x1000;
	u32int idx = INDEX_FROM_BIT(frame);
	u32int off = OFFSET_FROM_BIT(frame);
	return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static u32int first_frame()
{
	u32int i, j;
	for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
	{
		if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
		{
			// at least one bit is free here.
			for (j = 0; j < 32; j++)
			{
				u32int toTest = 0x1 << j;
				if ( !(frames[i]&toTest) )
				{
					return i*4*8+j;
				}
			}
		}
	}
	return 0;
}

// Function to allocate a frame.
void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
	if (page->frame != 0)
	{
		return;
	}
	else
	{
		u32int idx = first_frame();
		if (idx == (u32int)-1)
		{
			// PANIC! no free frames!!
			printf("*** BUG *** No more frames!\n");
			for(;;);
		}
		set_frame(idx*0x1000);
		page->present = 1;
		page->rw = (is_writeable)?1:0;
		page->user = (is_kernel)?0:1;
		page->frame = idx;
	}
}

// Function to deallocate a frame.
void free_frame(page_t *page)
{
	u32int frame;
	if (!(frame=page->frame))
	{
		return;
	}
	else
	{
		clear_frame(frame);
		page->frame = 0x0;
	}
}

page_directory_t *clone_directory(page_directory_t *src)
{
   u32int phys;
   // Make a new page directory and obtain its physical address.
   page_directory_t *dir = (page_directory_t*)kmalloc_ap(sizeof(page_directory_t), &phys);
   // Ensure that it is blank.
   _memset(dir, 0, sizeof(page_directory_t)); 
   // Get the offset of tablesPhysical from the start of the page_directory_t structure.
   u32int offset = (u32int)dir->tablesPhysical - (u32int)dir;

   // Then the physical address of dir->tablesPhysical is:
   dir->physicalAddr = phys + offset; 
   int i;
   for (i = 0; i < 1024; i++)
   {
       if (!src->tables[i])
           continue; 

        if (kernel_directory->tables[i] == src->tables[i])
        {
           // It's in the kernel, so just use the same pointer.
           dir->tables[i] = src->tables[i];
           dir->tablesPhysical[i] = src->tablesPhysical[i];
        }
        else
        {
           // Copy the table.
           u32int phys;
           dir->tables[i] = clone_table(src->tables[i], &phys);
           dir->tablesPhysical[i] = phys | 0x07;
        } 
   }
   return dir;
}

static page_table_t *clone_table(page_table_t *src, u32int *physAddr)
{
   // Make a new page table, which is page aligned.
   page_table_t *table = (page_table_t*)kmalloc_ap(sizeof(page_table_t), physAddr);
   // Ensure that the new table is blank.
   _memset(table, 0, sizeof(page_directory_t));

   // For every entry in the table...
   int i;
   for (i = 0; i < 1024; i++)
   {
       if (src->pages[i].frame)
       {
          // Get a new frame.
          alloc_frame(&table->pages[i], 0, 0);
          // Clone the flags from source to destination.
          if (src->pages[i].present) table->pages[i].present = 1;
          if (src->pages[i].rw)      table->pages[i].rw = 1;
          if (src->pages[i].user)    table->pages[i].user = 1;
          if (src->pages[i].accessed)table->pages[i].accessed = 1;
          if (src->pages[i].dirty)   table->pages[i].dirty = 1;
          // Physically copy the data across. This function is in process.s.
          copy_page_physical(src->pages[i].frame*0x1000, table->pages[i].frame*0x1000); 
       }
   }
   return table;
} 

u32int init_paging(void* mbd)
{
	// The size of physical memory. For the moment we 
	// assume it is 16MB big.
	//u32int mem_end_page = 0x1000000;
	u32int mem_end_page = ((long*)mbd)[2] * 1024;
	//u32int mem_end_page = 0x1000000;
	
	nframes = mem_end_page / 0x1000;
	frames = (u32int*)kmalloc(INDEX_FROM_BIT(nframes));
	_memset(frames, 0, INDEX_FROM_BIT(nframes));
	
	// Let's make a page directory.
	//u32int phys;
	kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
	_memset(kernel_directory, 0, sizeof(page_directory_t));
	kernel_directory->physicalAddr = (u32int)kernel_directory->tablesPhysical;

	// Map some pages in the kernel heap area.
	// Here we call get_page but not alloc_frame. This causes page_table_t's
	// to be created where necessary. We can't allocate frames yet because they
	// they need to be identity mapped first below, and yet we can't increase
	// placement_address between identity mapping and enabling the heap!
	int i = 0;
	for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000)
	   	get_page(i, 1, kernel_directory); 
	
	// We need to identity map (phys addr = virt addr) from
	// 0x0 to the end of used memory, so we can access this
	// transparently, as if paging wasn't enabled.
	// NOTE that we use a while loop here deliberately.
	// inside the loop body we actually change placement_address
	// by calling kmalloc(). A while loop causes this to be
	// computed on-the-fly rather than once at the start.
	i = 0;
	while (i < placement_address + 0x1000)
	{
		// Kernel code is readable but not writeable from userspace.
		alloc_frame( get_page(i, 1, kernel_directory), 0, 0);
		i += 0x1000;
	}
   
	// Now allocate those pages we mapped earlier.
	for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000)
		alloc_frame( get_page(i, 1, kernel_directory), 0, 0);

	// Before we enable paging, we must register our page fault handler.
	register_interrupt_handler(14, page_fault);

	// Now, enable paging!
	switch_page_directory(kernel_directory);

	// Initialise the kernel heap.
	kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0); 

	//printf("Here\n");
	
	current_directory = clone_directory(kernel_directory);
	switch_page_directory(current_directory);

	return mem_end_page - ((long*)mbd)[1] + 1024;
}

void switch_page_directory(page_directory_t *dir)
{
	current_directory = dir;
	asm volatile("mov %0, %%cr3":: "r"(dir->physicalAddr));
	u32int cr0;
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u32int address, int make, page_directory_t *dir)
{
	// Turn the address into an index.
	address /= 0x1000;
	// Find the page table containing this address.
	u32int table_idx = address / 1024;
	if (dir->tables[table_idx]) // If this table is already assigned
	{
		return &dir->tables[table_idx]->pages[address%1024];
	}
	else if(make)
	{
		u32int tmp;
		dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
		dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
		return &dir->tables[table_idx]->pages[address%1024];
	}
	else
	{
		return 0;
	}
}


void page_fault(registers_t regs)
{
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	u32int faulting_address;
	asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
	
	PANIC_BANNER;
	printf("Page fault accessing address 0x%x, EIP=0x%x: ", faulting_address, regs.eip);
	// The error code gives us details of what happened.
	int present   = !(regs.err_code & 0x1); // Page not present
	int rw = regs.err_code & 0x2;		   // Write operation?
	int us = regs.err_code & 0x4;		   // Processor was in user-mode?
	int reserved = regs.err_code & 0x8;	 // Overwritten CPU-reserved bits of page entry?
	int id = regs.err_code & 0x10;		  // Caused by an instruction fetch?

	if (present)
	{
		putstring(current_console, "present ");
	}
	if (rw)
	{
		putstring(current_console, "read-only ");
	}
	if (us)
	{
		putstring(current_console, "user-mode ");
	}
	if (reserved)
	{
		putstring(current_console, "reserved ");
	}
	printf("id=%d\n", id);
	backtrace(regs);
	blitconsole(current_console);
	asm volatile("cli");
	wait_forever();
}

