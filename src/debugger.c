#include <kernel.h>

static symbol_t* symbol_table = NULL;
static bool debug_signal = false;

#define BT_IRQ_MARKER ((void*)0xFFFFffffFFFFff01ull)

volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST,
	.revision = 0,
};

void gdb_decode(char* out, const uint8_t* in, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		out[i] = ((in[i] ^ 0x5A) - 7);
	}
	out[len] = '\0';
}

bool set_debug_signal(bool status)
{
	bool old = debug_signal;
	debug_signal = status;
	return old;
}

bool get_debug_signal()
{
	return debug_signal;
}

symbol_t* get_sym_table()
{
	return symbol_table;
}

void dump_hex(void* addr, uint64_t length)
{
	char line[MAX_STRINGLEN], part[MAX_STRINGLEN];
	unsigned char* address = addr;
	uint64_t index = 0;
	for(; index < length; index += 16) {
		*line = 0;
		snprintf(part, MAX_STRINGLEN, "%04lx: ", index);
		strlcat(line, part, MAX_STRINGLEN);
		size_t hex = 0;
		for (; hex < 16; ++hex) {
			if (index + hex < length) {
				snprintf(part, MAX_STRINGLEN, "%02X ", address[index + hex]);
			} else {
				snprintf(part, MAX_STRINGLEN, "   ");
			}
			strlcat(line, part, MAX_STRINGLEN);
		}
		snprintf(part, MAX_STRINGLEN, " | ");
		strlcat(line, part, MAX_STRINGLEN);
		for (hex = 0; hex < 16; ++hex) {
			if (index + hex < length) {
				snprintf(part, MAX_STRINGLEN, "%c", (address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);
			} else {
				snprintf(part, MAX_STRINGLEN, " ");
			}
			strlcat(line, part, MAX_STRINGLEN);
		}

		dprintf("%s\n", line);
	}
}

void symbol_fail()
{
	setforeground(COLOUR_DARKRED);
	putstring("Warning: Could not load /kernel.sym from boot device.\n");
	putstring("Debug symbols will be unavailable if there is a kernel panic.\n");
	setforeground(COLOUR_WHITE);
}

void init_debug()
{
	struct limine_module_response* mods = module_request.response;
	if (!mods || module_request.response->module_count == 0) {
		dprintf("SYMBOLS: No modules\n");
		symbol_fail();
		return;
	}
	int mod_index = -1;
	for (size_t n = 0; n < mods->module_count; ++n) {
		if (!strcmp(mods->modules[n]->path, "/kernel.sym")) {
			mod_index = n;
		}
	}
	if (mod_index == -1) {
		dprintf("SYMBOLS: No module found called 'Symbols'\n");
		symbol_fail();
		return;
	}
	uint64_t filesize = mods->modules[mod_index]->size;
	unsigned char* filecontent = mods->modules[mod_index]->address;
	unsigned char* ptr = filecontent;
	char symbol_address[32];
	char type[2];
	char symbol[1024];
	uint32_t counter = 0;
	uint32_t offset = 0;
	uint32_t symcount = 0;
	uint32_t sizebytes = 0;
	symbol_table = kmalloc(sizeof(symbol_t));
	symbol_t* thisentry = symbol_table;
	if (!symbol_table) {
		return;
	}

	while (offset < filesize) {
		counter = 0;
		while (offset < filesize && *ptr != ' ' && counter < sizeof(symbol_address) - 1) {
			symbol_address[counter++] = *ptr++;
			offset++;
		}
		symbol_address[counter] = 0;
		ptr++;
		offset++;

		counter = 0;
		while (offset < filesize && *ptr != ' ' && counter < 2) {
			type[counter++] = *ptr++;
			offset++;
		}
		type[1] = 0;
		ptr++;
		offset++;

		counter = 0;
		while (offset < filesize && *ptr != 0x0A && counter < sizeof(symbol) - 1) {
			symbol[counter++] = *ptr++;
			offset++;
		}
		symbol[counter] = 0;
		ptr++;
		offset++;

		uint32_t length = strlen(symbol) + 1;

		if (*type == 'T') {
			thisentry->name = kmalloc(length);
			if (!thisentry->name) {
				break;
			}
			memcpy(thisentry->name, symbol, length);
			thisentry->address = (uint64_t)atoll(symbol_address, 16);
			thisentry->type = *type;
			symbol_t* next = kmalloc(sizeof(symbol_t));
			if (!next) {
				kfree_null(&thisentry->name);
				break;
			}
			next->next = NULL;
			thisentry->next = next;
			thisentry = thisentry->next;
		}
		symcount++;
		sizebytes += sizeof(symbol_t) + length;
	}

	kprintf("Read ");
	setforeground(COLOUR_LIGHTYELLOW);
	kprintf("%d ", symcount);
	setforeground(COLOUR_WHITE);
	kprintf("symbols from ");
	setforeground(COLOUR_LIGHTYELLOW);
	kprintf("/kernel.sym ");
	setforeground(COLOUR_WHITE);
	kprintf("(%ld bytes)\n", filesize);
}

const char* findsymbol(uint64_t address, uint64_t* offset) {
	if (!symbol_table) {
		return NULL;
	}
	/* Only the bottom 32 bits of any address is respected here, because the symbol in the
	 * sym file might be relocated (higher half) from what is the real address.
	 */
	address = address & 0xffffffff;
	symbol_t* walksyms = symbol_table;
	uint64_t lastsymaddr = symbol_table->address;
	symbol_t* lastsym = symbol_table;
	for (; walksyms->next; walksyms = walksyms->next) {
		/* This check between the last address and this address works because the
		 * symbol list in /kernel.sym is sorted by address, lowest first. This makes
		 * it simpler to load and process the list at runtime by doing the donkey-
		 * work at compile-time.
		 */
		if (address >= lastsymaddr && address <= (walksyms->address & 0xffffffff)) {
			*offset = address - lastsymaddr;
			return lastsym->name;
		}
		lastsym = walksyms;
		lastsymaddr = walksyms->address & 0xffffffff;
	}
	return NULL;
}

/* reverse lookup: name -> address (64-bit kernel VA)
 * returns 0 if not found
 */
uint64_t findsymbol_addr(const char *name) {
	if (!symbol_table || !name) {
		return 0;
	}

	/* derive the current kernel high-half from any known kernel code pointer */
	const uint64_t kernel_hi = ((uint64_t)(uintptr_t)&findsymbol_addr) & 0xffffffff00000000ull;

	for (symbol_t *s = symbol_table; s; s = s->next) {
		if (!s->name) {
			continue;
		}
		if (strcmp(s->name, name) != 0) {
			continue;
		}

		uint64_t a = s->address;

		/* if the sym file only carried low 32 bits, graft on the current high half */
		if ((a & 0xffffffff00000000ull) == 0) {
			a = kernel_hi | (a & 0xffffffffull);
		}

		return a;
	}

	return 0;
}

static inline void get_kstack_bounds(uintptr_t *lo, uintptr_t *hi) {
	uintptr_t rsp;
	__asm__ volatile ("movq %%rsp,%0" : "=r"(rsp));
	uintptr_t base = rsp & KSTACK_MASK;
	*lo = base;
	*hi = base + KSTACK_SIZE;
}

static size_t count_markers_ahead(const stack_frame_t *frame, uintptr_t lo, uintptr_t hi, size_t probe) {
	size_t cnt = 0;
	while (frame && CANONICAL_ADDRESS(frame) && (uintptr_t)frame >= lo && (uintptr_t)frame <= (hi - sizeof(*frame)) && probe++ < 1024) {
		const stack_frame_t *next = frame->next;
		if (next && (uintptr_t)next <= (uintptr_t)frame) {
			break;
		}
		if (frame->addr == BT_IRQ_MARKER) {
			cnt++;
		}
		frame = next;
	}
	return cnt;
}

void backtrace(void) {
	stack_frame_t *frame;
	__asm__ volatile("movq %%rbp,%0" : "=r"(frame));
	uintptr_t lo, hi;
	size_t depth = 0;
	get_kstack_bounds(&lo, &hi);

	/* If we’re in an ISR/exception at the top */
	size_t remaining_markers = count_markers_ahead(frame, lo, hi, depth);
	uint32_t skip = 0;
	setforeground(COLOUR_LIGHTGREEN);

	while (frame && CANONICAL_ADDRESS(frame) && (uintptr_t)frame >= lo && (uintptr_t)frame <= (hi - sizeof(*frame)) && depth++ < 1024) {

		stack_frame_t *next = frame->next;

		/* always drop error_handler -> Interrupt -> interrupt_stub.
		 * Tracing through the error handler itself is not useful
		 */
		if (skip++ < 3) {
			frame = next;
			continue;
		}

		if (next && (uintptr_t)next <= (uintptr_t)frame) {
			break;
		} else if (frame->addr == BT_IRQ_MARKER) {
			if (remaining_markers > 0) {
				remaining_markers--;
			}
			setforeground(COLOUR_LIGHTYELLOW);
			if (remaining_markers > 0) {
				aprintf("--------------------------------[ IRQ/TRAP CONTEXT ]--------------------------------\n");
			} else {
				aprintf("-------------------------------[ PRE-EMPTED CONTEXT ]-------------------------------\n");
			}
			setforeground(COLOUR_LIGHTGREEN);
			frame = next;
			continue;
		}
		uint64_t offset = 0;
		const char *mname = NULL, *sname = NULL;
		if (module_addr_to_symbol(frame->addr, &mname, &sname, &offset)) {
			aprintf("\t%s:%s()+0%08lx [0x%lx]\n", mname, sname, offset, (uint64_t) frame->addr);
		} else {
			const char *name = findsymbol((uint64_t) frame->addr, &offset);
			aprintf("\t%s()+0%08lx [0x%lx]\n", name ? name : "[???]", offset, (uint64_t) frame->addr);
		}
		frame = next;
	}
	setforeground(COLOUR_WHITE);
}

uint32_t gdb_trace(const char* str) {
	uint32_t hash = 5381;
	uint8_t c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) ^ c;
	}
	return hash;
}

static const uint8_t gdb_proto_junk[][64] = {
	{ 0, 33, 50, 33, 54, 46, 54, 47, 33, 27, 125, 15, 42, 48, 54, 125, 33, 35, 218, 105, 125, 46, 54, 50, 33, 51, 50, 52, 111 },
	{ 12, 51, 32, 54, 35, 39, 50, 33, 42, 44, 47, 27, 125, 1, 53, 54, 125, 46, 54, 50, 33, 51, 50, 52, 125, 51, 54, 41, 42, 54, 39, 54, 32, 125, 45, 35, 42, 39, 42, 41, 54, 52, 54, 125, 42, 32, 125, 54, 50, 35, 47, 54, 49, 111 },
	{ 2, 38, 54, 35, 218, 27, 125, 17, 42, 49, 125, 218, 44, 38, 125, 33, 53, 42, 47, 40, 125, 33, 53, 42, 32, 125, 32, 218, 32, 33, 54, 46, 125, 53, 50, 49, 125, 54, 46, 45, 50, 33, 53, 218, 125, 32, 38, 51, 35, 44, 38, 33, 42, 47, 54, 32, 28 },
	{ 0, 33, 50, 33, 54, 46, 54, 47, 33, 27, 125, 58, 44, 38, 125, 50, 35, 54, 125, 50, 41, 35, 54, 50, 49, 218, 125, 50, 32, 125, 54, 32, 48, 50, 41, 50, 33, 54, 49, 125, 50, 32, 125, 218, 44, 38, 125, 46, 50, 218, 125, 51, 54, 105, 125, 46, 54, 50, 33, 51, 50, 52, 111 },
	{ 16, 41, 50, 35, 42, 55, 42, 48, 50, 33, 42, 44, 47, 27, 125, 1, 53, 54, 35, 54, 125, 42, 32, 125, 47, 44, 125, 116, 32, 38, 49, 44, 116, 111, 125, 12, 47, 41, 218, 125, 49, 54, 41, 38, 32, 42, 44, 47, 32, 125, 50, 47, 49, 125, 49, 54, 48, 50, 218, 111 },
	{ 4, 50, 35, 47, 42, 47, 52, 27, 125, 3, 54, 45, 54, 50, 33, 54, 49, 125, 50, 33, 33, 54, 46, 45, 33, 32, 125, 46, 50, 218, 125, 33, 35, 42, 52, 52, 54, 35, 125, 32, 50, 35, 48, 50, 32, 46, 125, 44, 39, 54, 35, 55, 41, 44, 36, 111 },
	{ 16, 44, 47, 48, 41, 38, 32, 42, 44, 47, 27, 125, 14, 54, 50, 33, 51, 50, 52, 32, 125, 48, 50, 47, 47, 44, 33, 125, 54, 32, 48, 50, 41, 50, 33, 54, 111, 125, 12, 47, 41, 218, 125, 54, 39, 50, 45, 44, 35, 50, 33, 54, 111 },
	{ 11, 38, 49, 52, 46, 54, 47, 33, 27, 125, 18, 48, 48, 54, 32, 32, 125, 49, 54, 47, 42, 54, 49, 111, 125, 15, 44, 33, 125, 49, 38, 54, 125, 33, 44, 125, 35, 42, 52, 53, 33, 32, 125, 65, 125, 49, 38, 54, 125, 33, 44, 125, 107, 218, 44, 38, 107, 111 },
	{ 18, 49, 39, 42, 32, 44, 35, 218, 27, 125, 10, 55, 125, 218, 44, 38, 125, 36, 50, 47, 33, 125, 45, 44, 36, 54, 35, 105, 125, 51, 38, 42, 41, 49, 125, 218, 44, 38, 35, 125, 44, 36, 47, 125, 12, 0, 105, 125, 46, 54, 50, 33, 51, 50, 52, 111 },
	{ 23, 50, 42, 41, 32, 50, 55, 54, 125, 33, 35, 42, 52, 52, 54, 35, 54, 49, 27, 125, 8, 54, 35, 47, 54, 41, 125, 45, 50, 47, 42, 48, 125, 46, 44, 49, 38, 41, 54, 125, 47, 44, 33, 125, 55, 44, 38, 47, 49, 111 }
};

static const size_t encoded_lengths[] = {
	29, 54, 57, 63, 60, 56, 53, 58, 56, 50
};

static size_t trace_count = 0;
static const size_t line_count = sizeof(encoded_lengths) / sizeof(encoded_lengths[0]);



void gdb_emit() {
	size_t idx = (trace_count < line_count)
		     ? (size_t)trace_count
		     : line_count - 1;

	char buf[MAX_DECODE];
	gdb_decode(buf, gdb_proto_junk[idx], encoded_lengths[idx]);

	setforeground(COLOUR_LIGHTRED);
	kprintf("%s\n", buf);
	setforeground(COLOUR_WHITE);

	trace_count++;
}
