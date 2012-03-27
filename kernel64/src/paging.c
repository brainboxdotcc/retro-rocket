#include <kernel.h>

// The kernel's page directory
page_directory_t* kernel_directory = 0;

// The current page directory;
page_directory_t* current_directory = 0;

// A bitset of frames - used or free.
u64 *frames;
u64 nframes;

extern u64 end;

u64 placement_address = (u64)&end;

u64 kmalloc_int(u64 sz, int align, u64 *phys)
{
    if (align == 1 && (placement_address & 0xFFF))
    {
	    placement_address &= 0xFFFFF000;
	    placement_address += 0x1000;
    }
    if (phys)
	    *phys = placement_address;
    u64 tmp = placement_address;
    placement_address += sz;
    return tmp;
}

u64 kmalloc_a(u64 sz)
{
	return kmalloc_int(sz, 1, 0);
}

u64 kmalloc_p(u64 sz, u64 *phys)
{
	return kmalloc_int(sz, 0, phys);
}

u64 kmalloc_ap(u64 sz, u64 *phys)
{
	return kmalloc_int(sz, 1, phys);
}

u64 kmalloc(u64 sz)
{
	return kmalloc_int(sz, 0, 0);
}

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the frames bitset
static void set_frame(u64 frame_addr)
{
    u64 frame = frame_addr/0x1000;
    u64 idx = INDEX_FROM_BIT(frame);
    u64 off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(u64 frame_addr)
{
    u64 frame = frame_addr/0x1000;
    u64 idx = INDEX_FROM_BIT(frame);
    u64 off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static u64 test_frame(u64 frame_addr)
{
    u64 frame = frame_addr/0x1000;
    u64 idx = INDEX_FROM_BIT(frame);
    u64 off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static u64 first_frame()
{
    u64 i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        if (frames[i] != 0xFFFFFFFFFFFFFFFF) // nothing free, exit early.
        {
            // at least one bit is free here.
            for (j = 0; j < 64; j++)
            {
                u64 toTest = 0x1 << j;
                if ( !(frames[i]&toTest) )
                {
                    return i*8*8+j;
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
        u64 idx = first_frame();
        if (idx == (u64)-1)
        {
            // PANIC! no free frames!!
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
    u64 frame;
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

void initialise_paging()
{
    // The size of physical memory. For the moment we 
    // assume it is 16MB big.
    u64 mem_end_page = 0x1000000;
    
    nframes = mem_end_page / 0x1000;
    frames = (u64*)kmalloc(INDEX_FROM_BIT(nframes));
    memset(frames, 0, INDEX_FROM_BIT(nframes));
    
    // Let's make a page directory.
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    current_directory = kernel_directory;

    // We need to identity map (phys addr = virt addr) from
    // 0x0 to the end of used memory, so we can access this
    // transparently, as if paging wasn't enabled.
    // NOTE that we use a while loop here deliberately.
    // inside the loop body we actually change placement_address
    // by calling kmalloc(). A while loop causes this to be
    // computed on-the-fly rather than once at the start.
    int i = 0;
    while (i < placement_address)
    {
	// Kernel code is readable but not writeable from userspace.
	alloc_frame( get_page(i, 1, kernel_directory), 0, 0);
	i += 0x1000;
    }

    // Now, enable paging!
    switch_page_directory(kernel_directory);
}

void switch_page_directory(page_directory_t *dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    u64 cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u64 address, int make, page_directory_t *dir)
{
    // Turn the address into an index.
    address /= 0x1000;
    // Find the page table containing this address.
    u64 table_idx = address / 1024;
    if (dir->tables[table_idx]) // If this table is already assigned
    {
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else if(make)
    {
        u64 tmp;
        dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
        dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else
    {
        return 0;
    }
}

