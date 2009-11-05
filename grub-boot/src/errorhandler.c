#include "../include/video.h"
#include "../include/interrupts.h"
#include "../include/errorhandler.h"
#include "../include/printf.h"
#include "../include/kernel.h"

void error_handler(registers_t regs);

void init_error_handler()
{
	int interrupt = 0;
	for (; interrupt < 19; ++interrupt)
		register_interrupt_handler(interrupt, error_handler);
}

void error_handler(registers_t regs)
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
	printf("Fatal exception %X: %s", regs.int_no, error_table[regs.int_no]);
	blitconsole(current_console);
	for(;;);
}


