#include <kernel.h>

static symbol_t* symbol_table = NULL;
uint32_t trace_thread_id = 0;
static bool debug_signal = false, debug_tracing = false;

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
	snprintf(p, strlen(packet) + 5, "%s#%02x", packet, checksum % 256);
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
		gdb_send_packet(src_ip, src_port, "$PacketSize=1000;qXfer:features:read+;qXfer:exec-file:read+;vContSupported+;multiprocess+");
	} else if (strcmp(cmd_first, "qTStatus") == 0) {
		gdb_send_packet(src_ip, src_port, "$T0;tnotrun:0");
	} else if (strcmp(cmd_first, "qTfV") == 0 || strcmp(cmd_first, "qTsV") == 0) {
		// index:value:isbuiltin:name-hex
		gdb_send_packet(src_ip, src_port, "$l");
	} else if (strcmp(cmd_first, "qfThreadInfo") == 0) {
		/* Thread ID list */
		gdb_send_packet(src_ip, src_port, "$m1,2,3,4,5");
	} else if (strcmp(cmd_first, "qsThreadInfo") == 0) {
		gdb_send_packet(src_ip, src_port, "$l");
	} else if (strcmp(cmd_first, "qXfer") == 0) {
		if (!strncmp(rest, ":exec-file:read:", 16)) {
			// :exec-file:read::0,ffb
			//                 ^ PID or empty
			gdb_send_packet(src_ip, src_port, "$m$l/programs/init");
		}
	}
}

void gdb_variable_command(uint32_t src_ip, uint16_t src_port, const char* command)
{
	char cmd_first[strlen(command)];
	int x = 0;
	const char* p = command;
	const char* rest = NULL;
	while (*p && *p != ';' && *p != '?') {
		cmd_first[x++] = *p++;
	}
	cmd_first[x] = 0;
	rest = p;
	dprintf("GDB vcmd: '%s' '%s'\n", cmd_first, rest);
	if (strcmp(cmd_first, "vMustReplyEmpty") == 0) {
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
	gdb_send_packet(src_ip, src_port, "$S02thread:p01.01");
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
		dprintf("GDB packet with invalid checksum: %02x vs %02x", their_sum, checksum);
	}
}

void debug_handle_packet(uint32_t src_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t length)
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
	unsigned char* address = addr;
	uint64_t index = 0;
	for(; index < length; index += 16) {
		dprintf("%04x: ", index);
		size_t hex = 0;
		for (; hex < 16; ++hex) {
			if (index + hex < length) {
				dprintf("%02X ", address[index + hex]);
			} else {
				dputstring("   ");
			}
		}
		dputstring(" | ");
		for (hex = 0; hex < 16; ++hex) {
			if (index + hex < length) {
				dput((address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);
			} else {
				dput(' ');
			}
		}

		dput('\n');
	}
}

void symbol_fail()
{
	setforeground(current_console, COLOUR_DARKRED);
	putstring(current_console, "Warning: Could not load /kernel.sym from boot device.\n");
	putstring(current_console, "Debug symbols will be unavailable if there is a kernel panic.\n");
	setforeground(current_console, COLOUR_WHITE);
}

void init_debug()
{
	fs_directory_entry_t* symfile = fs_get_file_info("/kernel.sym");
	if (!symfile) {
		symbol_fail();
		return;
	}
	uint32_t filesize = symfile->size;
	if (filesize == 0) {
		symbol_fail();
		return;
	} else {
		unsigned char* filecontent = kmalloc(filesize);
		if (!fs_read_file(symfile, 0, filesize, filecontent)) {
			symbol_fail();
			kfree(filecontent);
			return;
		}
		/* We now have the symbols in the filecontent buffer. Parse them to linked list,
		 * and free the buffer.
		 */
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
				memcpy(thisentry->name, symbol, length);
				thisentry->address = hextoint(symbol_address);
				thisentry->type = *type;
				symbol_t* next = kmalloc(sizeof(symbol_t));
				next->next = NULL;
				thisentry->next = next;
				thisentry = thisentry->next;
			}
			symcount++;
			sizebytes += sizeof(symbol_t) + length;
		}

		kfree(filecontent);
		kprintf("Read ");
		setforeground(current_console, COLOUR_LIGHTYELLOW);
		kprintf("%d ", symcount);
		setforeground(current_console, COLOUR_WHITE);
		kprintf("symbols from ");
		setforeground(current_console, COLOUR_LIGHTYELLOW);
		kprintf("/kernel.sym ");
		setforeground(current_console, COLOUR_WHITE);
		kprintf("(%d bytes)\n", filesize);
	}

	udp_register_daemon(DEBUG_DST_PORT, &debug_handle_packet);
}

const char* findsymbol(uint64_t address, uint64_t* offset)
{
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

void backtrace()
{
	stack_frame_t *frame;
	__asm__ volatile("movq %%rbp,%0" : "=r"(frame));
	uint64_t page = (uint64_t) frame & 0xFFFFFFFFFFFFF000ull;
	uint64_t offset = 0;
	const char* name = NULL;

	if (!symbol_table) {
		kprintf("No symbols available for backtrace\n");
		return;
	}

	/* Stack frame loop inspired by AlexExtreme's stack trace in Exclaim */
	setforeground(current_console, COLOUR_LIGHTGREEN);
	while (frame && ((uint64_t)frame & 0xFFFFFFFFFFFFF000ull) == page) {
		name = findsymbol((uint64_t)frame->addr, &offset);
		if (!name || (name && strcmp(name, "idt_init") && strcmp(name, "Interrupt") && strcmp(name, "error_handler"))) {
			kprintf("\tat %s()+0%08x [0x%llx]\n",  name ? name : "[???]", offset, frame->addr);
		}
		frame = frame->next;
	}
	setforeground(current_console, COLOUR_WHITE);
}

