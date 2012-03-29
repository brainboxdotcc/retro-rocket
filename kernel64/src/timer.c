#include <kernel.h>

u64 ticks = 0;
u32 timer_freq = 0;
u64 beep_end = 0;

void beep(u32 pitch)
{
	u32 Div;
	u8 tmp;
	
	Div = 1193180 / pitch;
	outb(0x43, 0xb6);
	outb(0x42, (u8) (Div) );
	outb(0x42, (u8) (Div >> 8));

	tmp = inb(0x61);
	if (tmp != (tmp | 3))
		outb(0x61, tmp | 3);

	// A beep is stopped by the timer interrupt half a second
	// after it has been started, asyncronously and automatically.
	beep_end = ticks + timer_freq / 2;
}
 
void stopbeep()
{
	u8 tmp = (inb(0x61) & 0xFC);
	outb(0x61, tmp);
	beep_end = 0;
}

void sleep_one_tick()
{
	u64 oldticks = ticks;
	while (oldticks != ticks);
}

u64 get_ticks()
{
	return ticks;
}

static void timer_callback()
{
	ticks++;
	if (beep_end != 0 && ticks > beep_end)
		stopbeep();
	//proc_timer();
}

void init_timer(u32 frequency)
{
	timer_freq = frequency;
	//register_interrupt_handler(IRQ0, &timer_callback);
	u32 divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	u8 l = (u8)(divisor & 0xFF);
	u8 h = (u8)( (divisor>>8) & 0xFF );
	outb(0x40, l);
	outb(0x40, h);
}

