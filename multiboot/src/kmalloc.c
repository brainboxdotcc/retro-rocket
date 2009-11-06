#include "../include/kernel.h"
#include "../include/kmalloc.h"


// end is defined in the linker script
extern u32int end;
u32int placement_address = (u32int)&end;

u32int kmalloc(u32int sz)
{
	u32int tmp = placement_address;
	placement_address += sz;
	return tmp;
}

u32int kmalloc_a(u32int sz, int align)
{
	if (align == 1 && (placement_address & 0xFFFFF000)) // If the address is not already page-aligned
	{
		// Align it.
		placement_address &= 0xFFFFF000;
		placement_address += 0x1000;
	}
	u32int tmp = placement_address;
	placement_address += sz;
	return tmp;
}

u32int kmalloc_p(u32int sz, int align, u32int *phys)
{
	if (align == 1 && (placement_address & 0xFFFFF000)) // If the address is not already page-aligned
	{
		// Align it.
		placement_address &= 0xFFFFF000;
		placement_address += 0x1000;
	}
	if (phys)
	{
		*phys = placement_address;
	}
	u32int tmp = placement_address;
	placement_address += sz;
	return tmp;
} 
