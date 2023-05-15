#include <kernel.h>
#include <homer.h>

void error_handler(uint8_t int_no, uint64_t errorcode, uint64_t irq_no, void* opaque);

void init_error_handler()
{
	int interrupt = 0;
	for (; interrupt < 19; ++interrupt) {
		register_interrupt_handler(interrupt, error_handler, dev_zero, NULL);
	}
}

void error_handler(uint8_t int_no, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq_no, void* opaque)
{
	static const char* const error_table[] = {
		"Division by zero exception",
		"Debug exception",
		"Non maskable interrupt",
		"Breakpoint exception",
		"Into detected overflow",
		"Out of bounds exception",
		"Invalid opcode exception",
		"Device not available exception",
		"Double fault",
		"Coprocessor segment overrun",
		"Invalid TSS",
		"Segment not present",
		"Stack segment fault",
		"General protection fault",
		"Page fault",
		"Unknown exception",
		"x87 floating point exception",
		"Alignment check exception",
		"Machine check exception",
		"SIMD floating point exception",
		"Virtualisation exception",
		"Control Protection exception",
		"Unknown exception",
		"Unknown exception",
		"Unknown exception",
		"Unknown exception",
		"Unknown exception",
		"Unknown exception",
		"Hypervisor injection exception",
		"VMM communication exception",
		"Security exception",
		"Unknown exception",
	};
	interrupts_off();
	setforeground(current_console, COLOUR_LIGHTWHITE);
	setbackground(current_console, COLOUR_BLACK);
	PANIC_BANNER;
	setforeground(current_console, COLOUR_LIGHTRED);
	kprintf("Fatal exception %02X: %s\n", int_no, error_table[int_no]);
	setforeground(current_console, COLOUR_WHITE);
	backtrace();
	wait_forever();
}


