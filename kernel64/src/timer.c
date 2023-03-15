#include <kernel.h>

volatile uint64_t ticks = 0;
volatile uint32_t timer_freq = 0;
volatile uint64_t beep_end = 0;

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
	// after it has been started, asyncronously and automatically.
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

void sleep(uint64_t secs)
{
	uint64_t start = ticks;
	uint64_t end = start + (secs * timer_freq);
	while (ticks < end)
		asm volatile("hlt");
}

uint64_t get_ticks()
{
	return ticks;
}

static void timer_callback(uint8_t isr, uint64_t errorcode, uint64_t irq)
{
	ticks++;

	if (beep_end != 0 && ticks > beep_end)
		stopbeep();
}

void init_timer(uint32_t frequency)
{
	timer_freq = frequency;
	register_interrupt_handler(IRQ0, &timer_callback);
	uint32_t divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );
	outb(0x40, l);
	outb(0x40, h);
}

