#include <kernel.h>
#include <ioapic.h>

#define IOAPIC_INT_UNMASK (1 << 16)

//#define GSI_OFFSET(gsi) (0x10 + gsi * 2)
#define GSI_OFFSET(ioapic, gsi) (0x10 + ((gsi - ioapic->gsi_base) * 2))

void ioapic_register_write(uint32_t index, uint32_t value, ioapic_t *ioapic)
{
	if (ioapic == NULL)
		return;
	volatile uint32_t *mmio = (volatile uint32_t*)ioapic->paddr;
	mmio[0] = index & 0xFF;
	mmio[4] = value;
}

uint32_t ioapic_register_read(uint32_t index, ioapic_t* ioapic)
{
	if (ioapic == NULL)
		return 0;
	volatile uint32_t *mmio = (volatile uint32_t*) ioapic->paddr;
	mmio[0] = index & 0xFF;
	volatile uint32_t ret = mmio[4];
	return ret;
}

ioapic_t* ioapic_find(uint32_t gsi)
{
	static ioapic_t ioapic_rec;
	uint16_t ioapic_count = get_ioapic_count();
	if (ioapic_count == 1) {
		ioapic_rec = get_ioapic(0);
		return &ioapic_rec;
	}
	int v = 0;
	for (; v < ioapic_count; ++v) {
		ioapic_rec = get_ioapic(v);
		if (gsi >= ioapic_rec.gsi_base && gsi < ioapic_rec.gsi_base + ioapic_rec.gsi_count) {
			return &ioapic_rec;
		}
	}
	return NULL;
}

void ioapic_write_gsi(ioapic_t* ioapic, uint32_t gsi, uint32_t lower, uint32_t upper)
{
	ioapic_register_write(GSI_OFFSET(ioapic, gsi), lower, ioapic);
	ioapic_register_write(GSI_OFFSET(ioapic, gsi) + 1, upper, ioapic);
}

void ioapic_read_gsi(ioapic_t* ioapic, uint32_t gsi, uint32_t* lower, uint32_t* upper)
{
	*lower = ioapic_register_read(GSI_OFFSET(ioapic, gsi), ioapic);
	*upper = ioapic_register_read(GSI_OFFSET(ioapic, gsi) + 1, ioapic);
}

void ioapic_mask_set(uint32_t gsi, bool masked) {
	uint32_t lower, upper;
	ioapic_t *ioapic = ioapic_find(gsi);
	ioapic_read_gsi(ioapic, gsi, &lower, &upper);

	if (masked) {
		lower |= (1 << 16);  // Set mask bit
	} else {
		lower &= ~(1 << 16); // Clear mask bit
	}

	ioapic_write_gsi(ioapic, gsi, lower, upper);
}

void ioapic_redir_set_precalculated(uint32_t gsi, uint32_t upper, uint32_t lower)
{
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
	ioapic_write_gsi(ioapic, gsi, lower, upper);
}

void ioapic_redir_set(uint32_t irq, uint32_t vector, uint32_t del_mode, uint32_t dest_mode, uint32_t intpol, uint32_t trigger_mode, uint32_t mask)
{
	uint32_t gsi = irq_to_gsi(irq);
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
	uint32_t lower = (vector & 0xff) | (del_mode << 8) | (dest_mode << 11) | (intpol << 13) | (trigger_mode << 15) | (mask << 16);
	uint32_t bsp_apic_id = get_lapic_id_from_cpu_id(logical_cpu_id());
	uint32_t upper = (bsp_apic_id << 24);
	ioapic_write_gsi(ioapic, gsi, lower, upper);
	dprintf("Remapping IRQ %d -> GSI %d -> Vector %d (trigger mode: %s, polarity: %s)\n", irq, gsi, vector,
		triggering_str(trigger_mode), polarity_str(trigger_mode));
}

// Unmask an interrupt on the IOAPIC
void ioapic_redir_unmask(uint32_t gsi)
{
	ioapic_t *ioapic = ioapic_find(gsi);
	if (ioapic == NULL) {
		return;
	}
	uint32_t lower, upper;
	ioapic_read_gsi(ioapic, gsi, &lower, &upper);
	lower &= ~IOAPIC_INT_UNMASK;
	ioapic_write_gsi(ioapic, gsi, lower, upper);
}

void ioapic_redir_get(uint32_t gsi, uint32_t* vector, uint32_t* del_mode, uint32_t* dest_mode, uint32_t* intpol, uint32_t* trigger_mode, uint32_t* mask, uint32_t* destination)
{
	ioapic_t* ioapic = ioapic_find(gsi);
	if (ioapic == NULL)
		return;
	uint32_t lower, upper;
	ioapic_read_gsi(ioapic, gsi, &lower, &upper);
	*vector = lower & 0xFF;
	*del_mode = (lower >> 8) & 7;
	*dest_mode = (lower >> 11) & 1;
	*intpol = (lower >> 13) & 1;
	*trigger_mode = (lower >> 15) & 1;
	*mask = (lower >> 16) & 1;
	*destination = (upper >> 24) & 0xFF;
}
