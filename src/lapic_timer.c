#include <kernel.h>

static const uint64_t quantum = 100;

void init_lapic_timer()
{
	if (quantum == 0) {
		return;
	}

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
	uint64_t ticks_per_second = 0xFFFFFFFF - apic_read(APIC_TMRCURRCNT);
	uint64_t ticks_per_quantum = ticks_per_second / quantum;

	add_random_entropy(ticks_per_second);

	dprintf("ticks_per_second = %lu; ticks_per_quantum = %lu; quantum = %lu\n", ticks_per_second, ticks_per_quantum, quantum);

	apic_write(APIC_TMRINITCNT, ticks_per_quantum);
	apic_write(APIC_LVT_TMR, IRQ16 | TMR_PERIODIC);
	apic_write(APIC_TMRDIV, 0x03);
}

