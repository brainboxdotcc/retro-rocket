#include <kernel.h>
#include <ioapic.h>

#define IOAPIC_INT_UNMASK 0b11111111111111101111111100000000
//#define IOAPIC_INT_UNMASK   0b11111111111111111111111111111111

void ioapic_register_write(uint32_t index, uint32_t value, ioapic_t *ioapic)
{
	if (ioapic == NULL)
		return;
	uint32_t *mmio = (uint32_t*)ioapic->paddr;
	mmio[0] = index & 0xFF;
	mmio[4] = value;
}

uint32_t ioapic_register_read(uint32_t index, ioapic_t* ioapic)
{
	if (ioapic == NULL)
		return 0;
	uint32_t *mmio = (uint32_t*) ioapic->paddr;
	mmio[0] = index & 0xFF;
	uint32_t ret = mmio[4];
	return ret;
}

ioapic_t* ioapic_find(uint32_t gsi)
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

void ioapic_redir_set_precalculated(uint32_t gsi, uint32_t upper, uint32_t lower)
{
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
        ioapic_register_write(0x10 + gsi * 2, lower, ioapic);
        ioapic_register_write(0x10 + gsi * 2 + 1, upper, ioapic);
}

void ioapic_redir_set(uint32_t gsi, uint32_t vector, uint32_t del_mode, uint32_t dest_mode, uint32_t intpol, uint32_t trigger_mode, uint32_t mask)
{
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
	uint32_t lower =
		(vector & 0xff) |
		((del_mode << 8) & 0b111) |
		((dest_mode << 11) & 0b1) |
		((intpol << 13) & 0b1) |
		((trigger_mode << 15) & 0b1) |
		((mask << 16) & 0b1);
	//uint32_t upper = (dest_mode << 24);
	uint32_t upper = 0;
	ioapic_register_write(0x10 + gsi * 2, lower, ioapic);
	ioapic_register_write(0x10 + gsi * 2 + 1, upper, ioapic);
}

// Unmask an interrupt on the IOAPIC and set its vector to GSI+32, e.g. IRQ0 = 32, IRQ1 = 33.
void ioapic_redir_unmask(uint32_t gsi)
{
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
	uint32_t lower = ioapic_register_read(0x10 + gsi * 2, ioapic);
        uint32_t upper = ioapic_register_read(0x10 + gsi * 2 + 1, ioapic);
	lower = lower & IOAPIC_INT_UNMASK;
	lower |= (gsi + 32);
	ioapic_register_write(0x10 + gsi * 2, lower, ioapic);
	ioapic_register_write(0x10 + gsi * 2 + 1, upper, ioapic);
}

void ioapic_redir_get(uint32_t gsi, uint32_t* vector, uint32_t* del_mode, uint32_t* dest_mode, uint32_t* intpol, uint32_t* trigger_mode, uint32_t* mask, uint32_t* destination)
{
	ioapic_t* ioapic = ioapic_find(gsi);
	if (ioapic == NULL)
		return;
	uint32_t lower = ioapic_register_read(0x10 + gsi * 2, ioapic);
	uint32_t upper = ioapic_register_read(0x10 + gsi * 2 + 1, ioapic);
	*vector = lower & 0xFF;
	*del_mode = (lower >> 8) & 0b111;
	*dest_mode = (lower >> 11) & 0b1;
	*intpol = (lower >> 13) & 0b1;
	*trigger_mode = (lower >> 15) & 0b1;
	*mask = (lower >> 16) & 0b1;
	*destination = (upper >> 24) & 0xFF;
}
