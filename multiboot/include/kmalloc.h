#ifndef __KMALLOC_H__
#define __KMALLOC_H__


/**
   Allocate a chunk of memory, sz in size. If align == 1,
   the chunk must be page-aligned. If phys != 0, the physical
   location of the allocated chunk will be stored into phys.

   This is the internal version of kmalloc. More user-friendly
   parameter representations are available in kmalloc, kmalloc_a,
   kmalloc_ap, kmalloc_p.
**/
u32int kmalloc_int(u32int sz, int align, u32int *phys);

/**
   Allocate a chunk of memory, sz in size. The chunk must be
   page aligned.
**/
u32int kmalloc_a(u32int sz);

/**
   Allocate a chunk of memory, sz in size. The physical address
   is returned in phys. Phys MUST be a valid pointer to u32int!
**/
u32int kmalloc_p(u32int sz, u32int *phys);

/**
   Allocate a chunk of memory, sz in size. The physical address 
   is returned in phys. It must be page-aligned.
**/
u32int kmalloc_ap(u32int sz, u32int *phys);

/**
   General allocation function.
**/
u32int kmalloc(u32int sz);

#endif
