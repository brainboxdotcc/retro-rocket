#include <kernel.h>

volatile uint64_t ticks = 0;
volatile uint32_t timer_freq = 1000;
volatile uint64_t beep_end = 0;

extern idle_timer_t* timer_idles;

void beep(uint32_t pitch)
{
	uint32_t Div;
	uint8_t tmp;
	
	Div = 1193180 / pitch;
	outb(0x43, 0xb6);
	outb(0x42, (uint8_t) (Div) );
	outb(0x42, (uint8_t) (Div >> 8));

	tmp = inb(0x61);
	if (tmp != (tmp | 3))
		outb(0x61, tmp | 3);

	// A beep is stopped by the timer interrupt half a second
	// after it has been started, asynchronously and automatically.
	beep_end = ticks + timer_freq / 8;
}
 
void stopbeep()
{
	uint8_t tmp = (inb(0x61) & 0xFC);
	outb(0x61, tmp);
	beep_end = 0;
}

void sleep_one_tick()
{
	uint64_t oldticks = ticks;
	while (oldticks != ticks);
}

void sleep(uint64_t milliseconds)
{
	uint64_t start = ticks;
	uint64_t end = start + milliseconds;
	while (ticks < end) {
		__asm__ volatile("hlt");
	}
}

uint64_t get_ticks()
{
	return ticks;
}

/**
 * This is triggered either by the Local APIC timer interrupt, or by the PIT.
 */
void timer_callback(uint8_t isr, uint64_t errorcode, uint64_t irq, void* opaque)
{
	if (logical_cpu_id() != 0) {
		/* Right now we do nothing on the APs, it's just to wake up HLT */
		return;
	}

	ticks++;

	for (idle_timer_t* i = timer_idles; i; i = i->next) {
		if (get_ticks() > i->next_tick) {
			i->func();
			i->next_tick = i->frequency + get_ticks();
		}
	}

	if (beep_end != 0 && ticks > beep_end) {
		stopbeep();
	}

	if (ticks % 10 && video_flip_auto()) {
		rr_flip();
	}
}
