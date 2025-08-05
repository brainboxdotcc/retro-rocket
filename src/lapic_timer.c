#include <kernel.h>

static inline uint32_t pm_timer_delta(uint32_t now, uint32_t start) {
	uint32_t mask = pm_timer_is_32_bit() ? 0xFFFFFFFF : 0xFFFFFF;
	return (now - start) & mask;
}

void init_lapic_timer(uint64_t quantum)
{
	if (quantum == 0) {
		return;
	}

	uint8_t cpu = logical_cpu_id();

	if (cpu == 0) {
		/* Only do this once */
		register_interrupt_handler(IRQ16, timer_callback, dev_zero, NULL);
	}

	// Divisor = 16
	apic_write(APIC_TMRDIV, 0x3);

	uint64_t ticks_per_second = 0;

	if (pm_timer_available()) {
		dprintf("CPU#%d APIC timer calibration (PM timer)...\n", cpu);

		apic_write(APIC_TMRINITCNT, 0xFFFFFFFF);

		uint32_t start_pm = pm_timer_read();
		uint32_t target_delta = 3579545 / 10;
		uint32_t now_pm, delta_pm;

		do {
			now_pm = pm_timer_read();
			delta_pm = pm_timer_delta(now_pm, start_pm);
		} while (delta_pm < target_delta);

		uint64_t lapic_elapsed = 0xFFFFFFFF - apic_read(APIC_TMRCURRCNT);

		ticks_per_second = (lapic_elapsed * 3579545ULL) / delta_pm;
	} else {
		dprintf("CPU#%d APIC timer calibration (RTC fallback)...\n", cpu);

		datetime_t t;
		get_datetime(&t);
		uint8_t secs = t.second;

		while (t.second == secs) {
			get_datetime(&t);
		}
		secs = t.second;
		apic_write(APIC_TMRINITCNT, 0xFFFFFFFF);
		while (t.second == secs) {
			get_datetime(&t);
		}
		ticks_per_second = 0xFFFFFFFF - apic_read(APIC_TMRCURRCNT);
	}

	uint64_t ticks_per_quantum = ticks_per_second / quantum;

	add_random_entropy(ticks_per_second);

	dprintf("ticks_per_second = %lu; ticks_per_quantum = %lu; quantum = %lu\n", ticks_per_second, ticks_per_quantum, quantum);

	apic_write(APIC_TMRINITCNT, ticks_per_quantum);
	apic_write(APIC_LVT_TMR, IRQ16 | TMR_PERIODIC);
	apic_write(APIC_TMRDIV, 0x03);
}
