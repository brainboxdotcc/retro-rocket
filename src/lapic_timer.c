#include <kernel.h>

void lapic_spurious([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq, void* opaque)
{
	return;
}

void init_lapic_timer(uint32_t quantum)
{
	if (quantum == 0) {
		return;
	}
	
	register_interrupt_handler(IRQ7, lapic_spurious, dev_zero, NULL);
	register_interrupt_handler(IRQ16, timer_callback, dev_zero, NULL);

	// Divisor = 16
	apic_write(APIC_TMRDIV, 0x3);

	datetime_t t;
	get_datetime(&t);
	uint8_t secs = t.second;

	// Busy-loop to calculate number of APIC timer ticks per second
	kprintf("APIC timer calibration... ");
	while (t.second == secs) {
		get_datetime(&t);
	}
	secs = t.second;
	apic_write(APIC_TMRINITCNT, 0xFFFFFFFF);
	while (t.second == secs) {
		get_datetime(&t);
	}
	kprintf("Done!\n");
	uint32_t ticks_per_second = 0xFFFFFFFF - apic_read(APIC_TMRCURRCNT);
	uint32_t ticks_per_quantum = ticks_per_second / quantum;

	//kprintf("ticks_per_second = %d; ticks_per_quantum = %d; quantum = %d\n", ticks_per_second, ticks_per_quantum, quantum);
	apic_write(APIC_TMRINITCNT, ticks_per_quantum);
	apic_write(APIC_LVT_TMR, IRQ16 | TMR_PERIODIC);
	apic_write(APIC_TMRDIV, 0x03);
}

