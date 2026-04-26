#include <kernel.h>
#include <cpuid.h>

static symbol_t* symbol_table = NULL;
static bool debug_signal = false;
static symbol_t** symbol_address_index = NULL;
static size_t symbol_address_count = 0;
static symbol_t** symbol_name_hash = NULL;
static size_t symbol_name_hash_size = 0;

#define BT_IRQ_MARKER ((void*)0xFFFFffffFFFFff01)
#define SYMBOL_HASH_SEED0 0x736f6d6570736575
#define SYMBOL_HASH_SEED1 0x646f72616e646f6d

static size_t symbol_next_pow2(size_t v)
{
	if (v <= 1) {
		return 1;
	}
	return 1ull << (64 - __builtin_clzll(v - 1));
}

static size_t symbol_hash_index(const char *name)
{
	return hashmap_sip(name, strlen(name), SYMBOL_HASH_SEED0, SYMBOL_HASH_SEED1) & (symbol_name_hash_size - 1);
}

static void symbol_build_indexes(size_t count)
{
	symbol_address_index = NULL;
	symbol_address_count = 0;
	symbol_name_hash = NULL;
	symbol_name_hash_size = 0;

	if (!symbol_table || count == 0) {
		return;
	}

	symbol_address_index = kmalloc(sizeof(symbol_t*) * count);
	if (!symbol_address_index) {
		return;
	}

	symbol_name_hash_size = symbol_next_pow2(count * 2);
	symbol_name_hash = kmalloc(sizeof(symbol_t*) * symbol_name_hash_size);
	if (!symbol_name_hash) {
		kfree_null(&symbol_address_index);
		symbol_name_hash_size = 0;
		return;
	}

	for (size_t n = 0; n < count; n++) {
		symbol_name_hash[n] = NULL;
	}

	for (size_t n = count; n < symbol_name_hash_size; n++) {
		symbol_name_hash[n] = NULL;
	}

	for (symbol_t *s = symbol_table; s && s->name; s = s->next) {
		symbol_address_index[symbol_address_count++] = s;

		size_t idx = symbol_hash_index(s->name);
		for (size_t probe = 0; probe < symbol_name_hash_size; probe++) {
			if (!symbol_name_hash[idx]) {
				symbol_name_hash[idx] = s;
				break;
			}

			idx++;
			if (idx == symbol_name_hash_size) {
				idx = 0;
			}
		}
	}
}

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

void dump_hex(const void* addr, uint64_t length)
{
	char line[MAX_STRINGLEN], part[MAX_STRINGLEN];
	unsigned const char* address = addr;
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

bool find_limine_module(const char* name, uint8_t** content, size_t* size) {
	struct limine_module_response* mods = module_request.response;
	if (!mods || module_request.response->module_count == 0) {
		return false;
	}
	for (size_t n = 0; n < mods->module_count; ++n) {
		if (!strcmp(mods->modules[n]->path, name)) {
			*size = mods->modules[n]->size;
			*content = mods->modules[n]->address;
			return true;
		}
	}
	return false;
}

void init_debug()
{
	unsigned char* c_ptr = NULL;
	size_t c_filesize = 0;
	unsigned char* ptr = NULL;
	uint32_t filesize = 0;
	if (!find_limine_module("/kernel.sym", &c_ptr, &c_filesize)) {
		symbol_fail();
		return;
	}
	if (!decompress_gzip(c_ptr, c_filesize, &ptr, &filesize)) {
		symbol_fail();
		return;
	}

	char type;
	uint32_t offset = 0;
	size_t text_symbol_count = 0;
	symbol_t* tail = NULL;
	uint8_t* file_end = ptr + filesize;
	symbol_table = NULL;

	while (offset < filesize) {
		uintptr_t addr = 0;
		while (*ptr != ' ' || ptr > file_end) {
			uint8_t c = tolower(*ptr++);
			addr <<= 4;
			if (c >= '0' && c <= '9') {
				addr |= (c - '0');
			} else if (c >= 'a' && c <= 'f') {
				addr |= (c - 'a' + 10);
			}
			offset++;
		}
		ptr++;
		offset++;

		type = *ptr;
		ptr += 2;
		offset += 2;

		uint8_t* end = memchr(ptr, 0x0A, filesize - offset);
		if (!end) {
			break;
		}
		size_t symbol_len = (size_t)(end - ptr);

		if (type == 'T') {
			symbol_t* entry = kmalloc(sizeof(symbol_t) + symbol_len + 1);
			if (!entry) {
				break;
			}
			memcpy(entry->name, ptr, symbol_len);
			entry->name[symbol_len] = 0;
			entry->address = addr;
			entry->type = type;
			entry->next = NULL;

			if (!tail) {
				symbol_table = entry;
			} else {
				tail->next = entry;
			}

			tail = entry;
			text_symbol_count++;
		}
		ptr = end + 1;
		offset += symbol_len + 1;
	}

	symbol_build_indexes(text_symbol_count);
	kprintf("Read \x1b[93m%lu\x1b[37m symbols from \x1b[93m/kernel.sym\x1b[37m (%u bytes)\n", text_symbol_count, filesize);
}

const char* findsymbol(uint64_t address, uint64_t* offset) {
	if (!symbol_address_index || symbol_address_count == 0) {
		return NULL;
	}
	/* Only the bottom 32 bits of any address is respected here, because the symbol in the
	 * sym file might be relocated (higher half) from what is the real address.
	 */
	address = address & 0xffffffff;

	size_t lo = 0;
	size_t hi = symbol_address_count;

	while (lo < hi) {
		size_t mid = lo + ((hi - lo) / 2);
		uint64_t midaddr = symbol_address_index[mid]->address & 0xffffffff;

		if (midaddr <= address) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	}

	if (lo == 0) {
		return NULL;
	}

	symbol_t *s = symbol_address_index[lo - 1];
	uint64_t symaddr = s->address & 0xffffffff;

	if (offset) {
		*offset = address - symaddr;
	}

	return s->name;
}

/* reverse lookup: name -> address (64-bit kernel VA)
 * returns 0 if not found
 */
uint64_t findsymbol_addr(const char *name) {
	if (!symbol_name_hash || symbol_name_hash_size == 0 || !name) {
		return 0;
	}

	/* derive the current kernel high-half from any known kernel code pointer */
	const uint64_t kernel_hi = ((uint64_t)(uintptr_t)&findsymbol_addr) & 0xffffffff00000000;

	size_t idx = symbol_hash_index(name);

	for (size_t probe = 0; probe < symbol_name_hash_size; probe++) {
		symbol_t *s = symbol_name_hash[idx];

		if (!s) {
			return 0;
		}

		if (strcmp(s->name, name) == 0) {
			uint64_t a = s->address;

			/* if the sym file only carried low 32 bits, graft on the current high half */
			if ((a & 0xffffffff00000000) == 0) {
				a = kernel_hi | (a & 0xffffffff);
			}

			return a;
		}

		idx++;
		if (idx == symbol_name_hash_size) {
			idx = 0;
		}
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

bool running_under_qemu(void)
{
	if (!cpu_caps.hypervisor_present) {
		return false;
	}

	return cpu_caps.hypervisor_vendor[0] == 'K'
	       && cpu_caps.hypervisor_vendor[1] == 'V'
	       && cpu_caps.hypervisor_vendor[2] == 'M';
}
