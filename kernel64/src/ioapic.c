#include <kernel.h>
#include <hydrogen.h>
#include <ioapic.h>

//#define IOAPIC_INT_UNMASK 0b11111111111111101111111100000000
#define IOAPIC_INT_UNMASK   0b11111111111111111111111111111111

void ioapic_register_write(u32 index, u32 value, ioapic_t *ioapic)
{
	if (ioapic == NULL)
		return;
	kprintf("ioapic_register_write(%d,%d)\n", index, value);
	u32 *mmio = (u32*)ioapic->paddr;
	mmio[0] = index & 0xFF;
	mmio[4] = value;
}

u32 ioapic_register_read(u32 index, ioapic_t* ioapic)
{
	kprintf("ioapic_register_read(%d,", index);
	if (ioapic == NULL)
		return 0;
	u32 *mmio = (u32*) ioapic->paddr;
	mmio[0] = index & 0xFF;
	u32 ret = mmio[4];
	kprintf("%016x) = %d\n", ioapic->paddr, ret);
	return ret;
}

ioapic_t* ioapic_find(u32 gsi)
{
	static ioapic_t ioapic_rec;
	uint16_t ioapic_count = get_ioapic_count();
	int v = 0;
	for (; v < ioapic_count; ++v) {
		ioapic_rec = get_ioapic(v);
		if (gsi >= ioapic_rec.gsi_base && gsi < ioapic_rec.gsi_base + ioapic_rec.gsi_count) {
			return &ioapic_rec;
		}
	}
	return NULL;
}

void ioapic_redir_set(u32 gsi, u32 vector, u32 del_mode, u32 dest_mode, u32 intpol, u32 trigger_mode, u32 mask)
{
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
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
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
	kprintf("ioapic_redir_unmask(%d)\n", gsi);
	u32 lower = ioapic_register_read(0x10 + gsi * 2, ioapic);
        u32 upper = ioapic_register_read(0x10 + gsi * 2 + 1, ioapic);
	kprintf("old upper: %08x, old lower: %08x\n", upper, lower);
	lower = lower & (IOAPIC_INT_UNMASK | (gsi + 32));
	kprintf("new lower: %08x mask %08x\n", lower, (IOAPIC_INT_UNMASK | (gsi + 32)));
	ioapic_register_write(0x10 + gsi * 2, lower, ioapic);
	ioapic_register_write(0x10 + gsi * 2 + 1, upper, ioapic);
}

void ioapic_redir_get(u32 gsi, u32* vector, u32* del_mode, u32* dest_mode, u32* intpol, u32* trigger_mode, u32* mask, u32* destination)
{
	ioapic_t* ioapic = ioapic_find(gsi);
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
