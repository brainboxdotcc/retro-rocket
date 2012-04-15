#include <kernel.h>

volatile u64 ticks = 0;
volatile u32 timer_freq = 0;
volatile u64 beep_end = 0;

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
	beep_end = ticks + timer_freq / 8;
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

void sleep(u64 secs)
{
	u64 start = ticks;
	u64 end = start + (secs * timer_freq);
	while (ticks < end)
		asm volatile("hlt");
}

u64 get_ticks()
{
	return ticks;
}

static void timer_callback(u8 isr, u64 errorcode, u64 irq)
{
	ticks++;
	if ((current_console && current_console->dirty) || (current_console != NULL && ticks % 10 == 0))
		blitconsole(current_console);

	if (beep_end != 0 && ticks > beep_end)
		stopbeep();
	//proc_timer();
}

void init_timer(u32 frequency)
{
	timer_freq = frequency;
	register_interrupt_handler(IRQ0, &timer_callback);
	u32 divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	u8 l = (u8)(divisor & 0xFF);
	u8 h = (u8)( (divisor>>8) & 0xFF );
	outb(0x40, l);
	outb(0x40, h);
}

