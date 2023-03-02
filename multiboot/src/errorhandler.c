#include "../include/video.h"
#include "../include/interrupts.h"
#include "../include/errorhandler.h"
#include "../include/kprintf.h"
#include "../include/kernel.h"
#include "../include/io.h"
#include "../include/debugger.h"

void error_handler(registers_t* regs);

void init_error_handler()
{
	int interrupt = 0;
	for (; interrupt < 14; ++interrupt)
		register_interrupt_handler(interrupt, error_handler);
	// Skip 14, page fault. This is used by the paging code.
	for (interrupt = 15; interrupt < 19; ++interrupt)
		register_interrupt_handler(interrupt, error_handler);
}

void error_handler(registers_t* regs)
{
	static const char* const error_table[] = {
		"Division by zero exception",
		"Debug exception",
		"Non maskable interrupt",
		"Breakpoint exception",
		"Into detected overflow",
		"Out of bounds exception",
		"Invalid opcode exception",
		"No coprocessor exception",
		"Double fault",
		"Coprocessor segment overrun",
		"Bad TSS",
		"Segment not present",
		"Stack fault",
		"General protection fault",
		"Page fault",
		"Unknown interrupt exception",
		"Coprocessor fault",
		"Alignment check exception",
		"Machine check exception",
	};
	PANIC_BANNER;
	kprintf("Fatal exception 0x%2x at 0x%8x: %s\n", regs->int_no, regs->eip, error_table[regs->int_no]);
	backtrace(regs);
	blitconsole(current_console);
	asm volatile("cli");
	wait_forever();
}


