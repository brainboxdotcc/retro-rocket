#include "../include/interrupts.h"
#include "../include/kprintf.h"
#include "../include/io.h"
#include "../include/kernel.h"
#include "../include/video.h"
#include "../include/taskswitch.h"

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

	// A beep is stopped by the timer interrupt half a second
	// after it has been started, asyncronously and automatically.
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

static void timer_callback(registers_t* regs)
{
	ticks++;
	if ((current_console && current_console->dirty) || (current_console != NULL && ticks % 10 == 0))
		blitconsole(current_console);
	if (beep_end != 0 && ticks > beep_end)
		stopbeep();
}

void init_timer(u32int frequency)
{
	timer_freq = frequency;
	register_interrupt_handler(IRQ0, &timer_callback);
	u32int divisor = 1193180 / frequency;
	outb(0x43, 0x36);
	u8int l = (u8int)(divisor & 0xFF);
	u8int h = (u8int)( (divisor>>8) & 0xFF );
	outb(0x40, l);
	outb(0x40, h);
} 
