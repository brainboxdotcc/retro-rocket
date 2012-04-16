#include <kernel.h>

void lapic_timer(u8 isr, u64 errorcode, u64 irq)
{
	//put(current_console, cpu_id() + 48);
	proc_timer();
}

void lapic_spurious(u8 isr, u64 errorcode, u64 irq)
{

	return;
}

void init_lapic_timer(u32 quantum)
{
	u32 tmp, cpubusfreq;

	register_interrupt_handler(IRQ7, lapic_spurious);
	register_interrupt_handler(IRQ16, lapic_timer);

	tmp = hydrogen_proc->lapic_freq / quantum / 16;

	apic_write(APIC_TMRINITCNT, tmp < 16 ? 16 : tmp);
	apic_write(APIC_LVT_TMR, IRQ16 | TMR_PERIODIC);
	apic_write(APIC_TMRDIV, 0x03);
}

