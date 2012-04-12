#include <kernel.h>

void error_handler(u8 int_no, u64 errorcode, u64 irq_no);

void init_error_handler()
{
	int interrupt = 0;
	for (; interrupt < 14; ++interrupt)
		register_interrupt_handler(interrupt, error_handler);
	// Skip 14, page fault. This is used by the paging code.
	for (interrupt = 15; interrupt < 19; ++interrupt)
		register_interrupt_handler(interrupt, error_handler);
}

void error_handler(u8 int_no, u64 errorcode, u64 irq_no)
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
	setforeground(COLOUR_LIGHTRED);
	kprintf("Fatal exception %02x: %s\n", int_no, error_table[int_no]);
	backtrace();
	asm volatile("cli");
	wait_forever();
}


