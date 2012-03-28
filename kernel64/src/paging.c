#include <kernel.h>

// The kernel's page directory
page_directory_t* kernel_directory = 0;

// The current page directory;
page_directory_t* current_directory = 0;

// A bitset of frames - used or free.
u64 *frames;
u64 nframes;

extern u64 k_end;

u64 placement_address = (u64)&k_end;

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
	return 0;
}

