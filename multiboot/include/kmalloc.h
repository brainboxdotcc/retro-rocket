#ifndef __KMALLOC_H__
#define __KMALLOC_H__

u32int kmalloc(u32int sz);
u32int kmalloc_a(u32int sz, int align);
u32int kmalloc_p(u32int sz, int align, u32int *phys);

#endif
