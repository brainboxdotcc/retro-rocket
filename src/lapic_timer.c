#include <kernel.h>

#define PM_HZ               3579545U
#define MIN_LAPIC_HZ        10000000ULL   /* 10 MHz floor */
#define MAX_CALIB_RETRIES   4

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

		uint32_t gate = PM_HZ / 10U;      /* ~100 ms */
		int attempt = 0;

		do {
			apic_write(APIC_TMRINITCNT, 0xFFFFFFFF);

			uint32_t start_pm = pm_timer_read();
			uint32_t now_pm;
			uint32_t delta_pm;

			do {
				now_pm = pm_timer_read();
				delta_pm = pm_timer_delta(now_pm, start_pm);
			} while (delta_pm < gate);

			uint64_t lapic_elapsed = 0xFFFFFFFFULL - (uint64_t)apic_read(APIC_TMRCURRCNT);
			ticks_per_second = (lapic_elapsed * PM_HZ) / delta_pm;

			if (ticks_per_second < MIN_LAPIC_HZ) {
				/* likely deschedule artefact; lengthen gate and try again (bounded) */
				attempt++;
				gate <<= 1; /* double to ~200 ms, 400 ms, ... */
			}
		} while (ticks_per_second < MIN_LAPIC_HZ && attempt < MAX_CALIB_RETRIES);

		/* final sanity clamp so we always tick */
		if (ticks_per_second < MIN_LAPIC_HZ) {
			dprintf("CPU#%d APIC timer: weak calibration (%lu Hz). Clamping to %lu Hz.\n", cpu, ticks_per_second, (uint64_t)MIN_LAPIC_HZ);
			ticks_per_second = MIN_LAPIC_HZ;
		}
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
	if (ticks_per_quantum == 0) {
		ticks_per_quantum = 1;
	}

	add_random_entropy(ticks_per_second);

	dprintf("ticks_per_second = %lu; ticks_per_quantum = %lu; quantum = %lu\n", ticks_per_second, ticks_per_quantum, quantum);

	apic_write(APIC_TMRINITCNT, ticks_per_quantum);
	apic_write(APIC_LVT_TMR, IRQ16 | TMR_PERIODIC);
	apic_write(APIC_TMRDIV, 0x03);
}
