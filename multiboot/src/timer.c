#include "../include/interrupts.h"
#include "../include/printf.h"
#include "../include/io.h"
#include "../include/kernel.h"
#include "../include/video.h"

u32int ticks = 0;
u32int timer_freq = 0;
u32int beep_end = 0;

void beep(u32int pitch)
{
	u32int Div;
	u8int tmp;
	
	Div = 1193180 / pitch;
	outb(0x43, 0xb6);
	outb(0x42, (u8int) (Div) );
	outb(0x42, (u8int) (Div >> 8));

	tmp = inb(0x61);
	if (tmp != (tmp | 3))
		outb(0x61, tmp | 3);

	beep_end = ticks + timer_freq / 2;
}
 
void stopbeep()
{
	u8int tmp = (inb(0x61) & 0xFC);
	outb(0x61, tmp);
	beep_end = 0;
}

void sleep_one_tick()
{
	u32int oldticks = ticks;
	while (oldticks != ticks);
}

static void timer_callback(registers_t regs)
{
	ticks++;
	if (current_console && current_console->dirty)
		blitconsole(current_console);
	if (beep_end != 0 && ticks > beep_end)
		stopbeep();
}

void init_timer(u32int frequency)
{
	timer_freq = frequency;

	// Firstly, register our timer callback.
	register_interrupt_handler(IRQ0, &timer_callback);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	u32int divisor = 1193180 / frequency;

	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	u8int l = (u8int)(divisor & 0xFF);
	u8int h = (u8int)( (divisor>>8) & 0xFF );

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);
} 
