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

/* Tiny decoder for exceptions that carry an error code */
static inline const char* print_error_code_detail(uint64_t vec, uint64_t ec) {
	static char message[MAX_STRINGLEN];
	switch (vec) {
		case 10: /* #TS */
		case 11: /* #NP */
		case 12: /* #SS */
		case 13: { /* #GP */
			const unsigned ext = (ec >> 0) & 1; /* set if originated externally */
			const unsigned idt = (ec >> 1) & 1; /* 1 => refers to IDT gate */
			const unsigned ti  = (ec >> 2) & 1; /* 0=GDT, 1=LDT (ignored if idt=1) */
			const unsigned idx = (unsigned)((ec >> 3) & 0x1FFF); /* selector index */

			const char *table = idt ? "IDT" : (ti ? "LDT" : "GDT");
			snprintf(message, MAX_STRINGLEN, "idx=%u, table=%s%s", idx, table, ext ? ", ext" : "");
			break;
		}
		case 14: { /* #PF: Page Fault */
			const int p   = (ec >> 0) & 1; /* 0=not-present, 1=protection */
			const int wr  = (ec >> 1) & 1; /* 1=write, 0=read */
			const int us  = (ec >> 2) & 1; /* 1=user, 0=kernel */
			const int rsv = (ec >> 3) & 1; /* reserved bit violation */
			const int ift = (ec >> 4) & 1; /* instruction fetch */
			const int pkey= (ec >> 5) & 1; /* protection key (if enabled) */
			const int sstk= (ec >> 6) & 1; /* CET shadow stack (if enabled) */
			/* bit 7 (HLAT) exists on some parts; ignore silently if 0 */

			snprintf(message, MAX_STRINGLEN, "%s, %s, %s%s%s%s%s",
				p ? "protection" : "not-present",
				wr ? "write" : "read",
				us ? "user" : "kernel",
				rsv ? ", rsvd-bit" : "",
				ift ? ", ifetch" : "",
				pkey ? ", pkey" : "",
				sstk ? ", sstk" : "");
			break;
		}
		default:
			snprintf(message, MAX_STRINGLEN, "no error detail");
			break;
	}
	return message;
}

void error_handler(uint8_t int_no, uint64_t errorcode, uint64_t rip, void* opaque) {
	interrupts_off();
	setforeground(COLOUR_LIGHTRED);
	setbackground(COLOUR_BLACK);
	const char* log = tail_last_lines(dprintf_buffer_snapshot(), 15);
	aprintf("\nFatal exception %02X (Error code %016lx): %s (%s)\n", int_no, errorcode, error_table[int_no], print_error_code_detail(int_no, errorcode));
	if (rip) {
		uint64_t offset = 0;
		const char *mname = NULL, *sname = NULL;
		if (module_addr_to_symbol(rip, &mname, &sname, &offset)) {
			/* print "mname:sname+off" (or mname:[???]) */
			aprintf("\tfault @ %s:%s()+0%08lx [0x%lx]\n", mname, sname, offset, (uint64_t) rip);
		} else {
			const char *name = findsymbol(rip, &offset);
			aprintf("\tfault @ %s()+0%08lx [0x%lx]\n", name ? name : "[???]", offset, (uint64_t) rip);
		}
	}
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


