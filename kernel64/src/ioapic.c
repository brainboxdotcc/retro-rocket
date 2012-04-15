#include <kernel.h>
#include <hydrogen.h>
#include <ioapic.h>

#define IOAPIC_INT_UNMASK 0b11111111111111101111111100000000

void ioapic_register_write(u32 index, u32 value, HydrogenInfoIOAPIC *ioapic)
{
	u32 *mmio = (u32*)ioapic->paddr;
	mmio[0] = index;
	mmio[0x10 / 4] = value;
}

u32 ioapic_register_read(u32 index, HydrogenInfoIOAPIC* ioapic)
{
	u32 *mmio = (u32*) ioapic->paddr;
	mmio[0] = index;
	return mmio[0x10 / 4];
}

HydrogenInfoIOAPIC* ioapic_find(u32 gsi)
{
	return hydrogen_ioapic;
	kprintf("Looking for gsi %d\n", gsi);
	HydrogenInfoIOAPIC* cur = hydrogen_ioapic;
	u32 count = 0;
	while (count++ < hydrogen_info->ioapic_count)
	{
		kprintf("IOAPIC base %d count %d\n", cur->gsi_base, cur->gsi_count);
		if (cur->gsi_base >= gsi && cur->gsi_base + cur->gsi_count < gsi)
		{
			kprintf("Found apic\n");
			return cur;
		}

		cur = (HydrogenInfoIOAPIC*)((u64)cur + (u64)sizeof(HydrogenInfoIOAPIC));
	}
	return NULL;
}

void ioapic_redir_set(u32 gsi, u32 vector, u32 del_mode, u32 dest_mode, u32 intpol, u32 trigger_mode, u32 mask)
{
	HydrogenInfoIOAPIC *ioapic = ioapic_find(gsi);
	if (ioapic == NULL)
		return;
	u32 lower =
		(vector & 0xff) |
		((del_mode << 8) & 0b111) |
		((dest_mode << 11) & 0b1) |
		((intpol << 13) & 0b1) |
		((trigger_mode << 15) & 0b1) |
		((mask << 16) & 0b1);
	u32 upper = (dest_mode << 24);
	kprintf("upper=%08x lower=%08x %d %d %d %d %d %d\n", upper, lower, vector, del_mode, dest_mode, intpol, trigger_mode, mask);
	ioapic_register_write(0x10 + gsi * 2, lower, ioapic);
	ioapic_register_write(0x10 + gsi * 2 + 1, upper, ioapic);
}

// Unmask an interrupt on the IOAPIC and set its vector to GSI+32, e.g. IRQ0 = 32, IRQ1 = 33.
void ioapic_redir_unmask(u32 gsi)
{
	HydrogenInfoIOAPIC *ioapic = ioapic_find(gsi);
	u32 lower = ioapic_register_read(0x10 + gsi * 2, ioapic);
        u32 upper = ioapic_register_read(0x10 + gsi * 2 + 1, ioapic);
	lower = lower & (IOAPIC_INT_UNMASK | (gsi + 32));
	ioapic_register_write(0x10 + gsi * 2, lower, ioapic);
	ioapic_register_write(0x10 + gsi * 2 + 1, upper, ioapic);
}

void ioapic_redir_get(u32 gsi, u32* vector, u32* del_mode, u32* dest_mode, u32* intpol, u32* trigger_mode, u32* mask, u32* destination)
{
	HydrogenInfoIOAPIC* ioapic = ioapic_find(gsi);
	if (ioapic == NULL)
		return;
	u32 lower = ioapic_register_read(0x10 + gsi * 2, ioapic);
	u32 upper = ioapic_register_read(0x10 + gsi * 2 + 1, ioapic);
	kprintf("rupper=%08x rlower=%08x\n", upper, lower);
	*vector = lower & 0xFF;
	*del_mode = (lower >> 8) & 0b111;
	*dest_mode = (lower >> 11) & 0b1;
	*intpol = (lower >> 13) & 0b1;
	*trigger_mode = (lower >> 15) & 0b1;
	*mask = (lower >> 16) & 0b1;
	*destination = (upper >> 24) & 0xFF;
}
