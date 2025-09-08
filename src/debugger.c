#include <kernel.h>

static symbol_t* symbol_table = NULL;
uint32_t trace_thread_id = 0;
static bool debug_signal = false;

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


void gdb_send_ack(uint32_t src_ip, uint16_t src_port)
{
	src_ip = htonl(src_ip);
	udp_send_packet((uint8_t*)&src_ip, DEBUG_DST_PORT, src_port, "+", 1);
}

void gdb_send_packet(uint32_t src_ip, uint16_t src_port, const char* packet)
{
	src_ip = htonl(src_ip);
	uint64_t checksum = 0;
	const uint8_t* ptr = (const uint8_t*)packet + 1;
	while (*ptr) {
		checksum += *ptr++;
	}
	char p[strlen(packet) + 5];
	snprintf(p, strlen(packet) + 5, "%s#%02x", packet, (uint8_t)checksum % 256);
	udp_send_packet((uint8_t*)&src_ip, DEBUG_DST_PORT, src_port, p, strlen(p));
}

void gdb_send_nack(uint32_t src_ip, uint16_t src_port)
{
	src_ip = htonl(src_ip);
	udp_send_packet((uint8_t*)&src_ip, DEBUG_DST_PORT, src_port, "-", 1);
}

void gdb_ack(uint32_t src_ip, uint16_t src_port)
{
}

void gdb_retransmit(uint32_t src_ip, uint16_t src_port)
{
}

void gdb_query(uint32_t src_ip, uint16_t src_port, const char* command)
{
	char cmd_first[strlen(command)];
	int x = 0;
	const char* p = command;
	const char* rest = NULL;
	while (*p && *p != ';' && *p != ':' && *p != '?') {
		cmd_first[x++] = *p++;
	}
	cmd_first[x] = 0;
	rest = p;
	dprintf("GDB qcmd: '%s' '%s'\n", cmd_first, rest);
	if (strcmp(cmd_first, "qSupported") == 0) {
		//gdb_send_packet(src_ip, src_port, "$QSupported:multiprocess+;qXfer:exec-file:read;swbreak+;hwbreak+;fork-events+;exec-events+;vContSupported+;no-resumed+;memory-tagging+;xmlRegisters=i386");
		gdb_send_packet(src_ip, src_port, "$PacketSize=1000;QNonStop-;QDisableRandomization+;qXfer:threads:read+;qXfer:features:read-;qXfer:exec-file:read+;vContSupported+;multiprocess+");
	} else if (strcmp(cmd_first, "qTStatus") == 0) {
		// Thread status
		gdb_send_packet(src_ip, src_port, "$T0;tnotrun:0;tframes:0;tcreated:0;tfree:50*!;tsize:50*!;circular:0;disconn:0;starttime:0;stoptime:0;username:;notes::");
	} else if (strcmp(cmd_first, "qTfV") == 0 || strcmp(cmd_first, "qTsV") == 0) {
		// get list of trace vars
		gdb_send_packet(src_ip, src_port, "$l");
	} else if (strcmp(cmd_first, "qTfP") == 0 || strcmp(cmd_first, "qTsP") == 0) {
		// get list of tracepoints
		gdb_send_packet(src_ip, src_port, "$l");
	} else if (strcmp(cmd_first, "qfThreadInfo") == 0) {
		/* Thread ID list */
		gdb_send_packet(src_ip, src_port, "$m1,2,3,4,5");
	} else if (strcmp(cmd_first, "qsThreadInfo") == 0) {
		gdb_send_packet(src_ip, src_port, "$l");
	} else if (strcmp(cmd_first, "qAttached") == 0) {
		// 1 if attached successfully to process, 0 if started a new process, Exx on error
		gdb_send_packet(src_ip, src_port, "$1");
	} else if (strcmp(cmd_first, "qC") == 0) {
		// Return current thread id
		gdb_send_packet(src_ip, src_port, "$QCp1.1");
	} else if (strcmp(cmd_first, "qXfer") == 0) {
		if (!strncmp(rest, ":exec-file:read:", 16)) {
			gdb_send_packet(src_ip, src_port, "$l/programs/init");
		} else if (!strncmp(rest, ":threads:read:", 14)) {
			gdb_send_packet(src_ip, src_port, "$l<threads><thread id=\"p1.1\" core=\"0\" name=\"/programs/init\"/></threads>");
		}
	}
}

void gdb_variable_command(uint32_t src_ip, uint16_t src_port, const char* command)
{
	char cmd_first[strlen(command)];
	int x = 0;
	const char* p = command;
	const char* rest = NULL;
	while (*p && *p != ';' && *p != '?' && *p != ':') {
		cmd_first[x++] = *p++;
	}
	cmd_first[x] = 0;
	rest = p;
	dprintf("GDB vcmd: '%s' '%s'\n", cmd_first, rest);
	if (strcmp(cmd_first, "vMustReplyEmpty") == 0) {
		gdb_send_packet(src_ip, src_port, "$");
	} else if (strcmp(cmd_first, "vFile") == 0) {
		// Host I/O: Not currently supported as it tries to download current executable!
		gdb_send_packet(src_ip, src_port, "$");
	}
}

void gdb_set_thread(uint32_t src_ip, uint16_t src_port, const char* command)
{
	const char op = command[1];
	const char* thread_id = command + 2;
	trace_thread_id = atoi(thread_id);
	dprintf("GDB: set thread: op=%c thread_id=%s\n", op, thread_id);
	gdb_send_packet(src_ip, src_port, "$OK");
}

void gdb_status_query(uint32_t src_ip, uint16_t src_port, const char* command)
{
	gdb_send_packet(src_ip, src_port, "$S01thread:p1.1");
}

void gdb_regs(uint32_t src_ip, uint16_t src_port, const char* command)
{
	gdb_send_packet(src_ip, src_port, "$xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
}

void gdb_reg(uint32_t src_ip, uint16_t src_port, const char* command)
{
	if (*(command + 1) == '8') {
		/* Program counter */
		gdb_send_packet(src_ip, src_port, "$00000010");
		return;
	}
	gdb_send_packet(src_ip, src_port, "$xxxxxxxx");
}

void gdb_mem(uint32_t src_ip, uint16_t src_port, const char* command)
{
	uint64_t count = hextoint(strchr(command + 1, ',') + 1);
	char out[count * 2 + 2];
	memset(out, '0', count * 2 + 2);
	out[count * 2 + 1] = 0;
	out[0] = '$';
	dprintf("MEM (%d): %s\n", strlen(out), out);
	gdb_send_packet(src_ip, src_port, out);
}

void gdb_command(uint32_t src_ip, uint16_t src_port, const char* command)
{
	dprintf("GDB: from '%04x:%04x' '%s'\n", src_ip, src_port, command);
	gdb_send_ack(src_ip, src_port);
	switch (*command) {
		case 'q':
			return gdb_query(src_ip, src_port, command);
		case 'v':
			return gdb_variable_command(src_ip, src_port, command);
		case 'H':
			return gdb_set_thread(src_ip, src_port, command);
		case '?':
			return gdb_status_query(src_ip, src_port, command);
		case 'g':
			return gdb_regs(src_ip, src_port, command);
		case 'p':
			return gdb_reg(src_ip, src_port, command);
		case 'm':
			return gdb_mem(src_ip, src_port, command);
	}
}

void gdb_data(uint32_t src_ip, uint16_t src_port, uint8_t* data, uint32_t length)
{
	uint64_t checksum = 0, n = 0, their_sum = 0;
	uint8_t* ptr = data;
	char xsum[4] = { 0 };
	char commands[length];
	while (n < length && *ptr != '#') {
		checksum += *ptr++;
		++n;
	}
	++ptr;
	strlcpy(xsum, (const char*)ptr, 3);
	their_sum = hextoint(xsum);
	checksum %= 256;
	if (checksum == their_sum) {
		strlcpy(commands, (const char*)data, n + 1);
		gdb_command(src_ip, src_port, commands);
	} else {
		dprintf("GDB packet with invalid checksum: %lx vs %lx", their_sum, checksum);
	}
}

void debug_handle_packet(uint32_t src_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t length, void* opaque)
{
	uint8_t identifier = *(uint8_t*)data;
	switch (identifier) {
		case '+':
			return gdb_ack(src_ip, src_port);
		case '-':
			return gdb_retransmit(src_ip, src_port);
		case '$':
			return gdb_data(src_ip, src_port, data + 1, length - 1);
	}
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
			thisentry->address = hextoint(symbol_address);
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

	udp_register_daemon(DEBUG_DST_PORT, &debug_handle_packet, NULL);
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


void backtrace()
{
	stack_frame_t *frame;
	__asm__ volatile("movq %%rbp,%0" : "=r"(frame));
	uint64_t page = (uint64_t) frame & 0xFFFFFFFFFFFFF000ull;
	uint64_t offset = 0;
	const char* name = NULL;

	/* Stack frame loop inspired by AlexExtreme's stack trace in Exclaim */
	setforeground(COLOUR_LIGHTGREEN);
	while (frame && ((uint64_t)frame & 0xFFFFFFFFFFFFF000ull) == page) {
		name = findsymbol((uint64_t)frame->addr, &offset);
		if (!name || (strcmp(name, "pci_enable_msi") != 0 && strcmp(name, "vprintf") != 0 && strcmp(name, "printf") != 0)) {
			kprintf("\tat %s()+0%08lx [0x%lx]\n",  name ? name : "[???]", offset, (uint64_t)frame->addr);
		}
		frame = frame->next;
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
