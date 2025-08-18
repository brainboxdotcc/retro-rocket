#include <kernel.h>
#include <homer.h>

const char* const error_table[] = {
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

void error_handler(uint8_t int_no, uint64_t errorcode, uint64_t irq_no, void* opaque);

void init_error_handler()
{
	for (int interrupt = 0; interrupt < 32; ++interrupt) {
		register_interrupt_handler(interrupt, error_handler, dev_zero, NULL);
	}
	dprintf("Init error handlers\n");
}

void error_handler(uint8_t int_no, uint64_t errorcode, [[maybe_unused]] uint64_t irq_no, void* opaque)
{
	interrupts_off();
	setforeground(COLOUR_LIGHTWHITE);
	setbackground(COLOUR_BLACK);
	const char* log = dprintf_buffer_snapshot();
	if (log) {
		kprintf("\n%s\n", log);
	}
	setforeground(COLOUR_LIGHTRED);
	kprintf("Fatal exception %02X (Error code %016lx): %s\n", int_no, errorcode, error_table[int_no]);
	setforeground(COLOUR_WHITE);
	backtrace();
	rr_flip();
#ifdef PROFILE_KERNEL
	profile_dump();
#endif
	wait_forever();
}


