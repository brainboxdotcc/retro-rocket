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

void init_error_handler() {
	for (int interrupt = 0; interrupt < 32; ++interrupt) {
		register_interrupt_handler(interrupt, error_handler, dev_zero, NULL);
	}
	dprintf("Init error handlers\n");
}

#include <stddef.h>
#include <string.h>

const char* tail_last_lines(const char *s, size_t n) {
	if (s == NULL) {
		return NULL;
	}
	if (n == 0) {
		return s + strlen(s);
	}
	const char *end = s + strlen(s);
	const char *p = end;
	if (p > s && *(p - 1) == '\n') {
		p--;
	}
	size_t need = n;
	while (p > s) {
		p--;
		if (*p == '\n') {
			if (--need == 0) {
				return p + 1;
			}
		}
	}
	return s;
}


void error_handler(uint8_t int_no, uint64_t errorcode, uint64_t irq_no, void* opaque) {
	interrupts_off();
	setforeground(COLOUR_LIGHTRED);
	setbackground(COLOUR_BLACK);
	const char* log = tail_last_lines(dprintf_buffer_snapshot(), 15);
	kprintf("\nFatal exception %02X (Error code %016lx): %s\n", int_no, errorcode, error_table[int_no]);
	dprintf("\nFatal exception %02X (Error code %016lx): %s\n", int_no, errorcode, error_table[int_no]);
	setforeground(COLOUR_WHITE);
	kprintf("------------------------------------[ DEBUG LOG ]-----------------------------------\n");
	setforeground(COLOUR_GREY);
	if (log) {
		kprintf("%s\n", log);
	}
	setforeground(COLOUR_WHITE);
	backtrace();
	rr_flip();
#ifdef PROFILE_KERNEL
	profile_dump();
#endif
	wait_forever();
}


