#include <kernel.h>

void lapic_timer(uint8_t isr, uint64_t errorcode, uint64_t irq)
{
	//put(current_console, cpu_id() + 48);
	//proc_timer();
}

void lapic_spurious(uint8_t isr, uint64_t errorcode, uint64_t irq)
{

	return;
}

void init_lapic_timer(uint32_t quantum)
{
	uint32_t tmp;

	register_interrupt_handler(IRQ7, lapic_spurious);
	register_interrupt_handler(IRQ16, lapic_timer);

	//tmp = hydrogen_proc->lapic_freq / quantum / 16;
	tmp = 0; // placeholder

	apic_write(APIC_TMRINITCNT, tmp < 16 ? 16 : tmp);
	apic_write(APIC_LVT_TMR, IRQ16 | TMR_PERIODIC);
	apic_write(APIC_TMRDIV, 0x03);
}

