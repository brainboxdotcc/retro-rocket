/**
 * @file basic/sockets.c
 * @brief BASIC network socket functions
 */
#include <kernel.h>

static queued_udp_packet* udp_packets[65536] = {0};
static queued_udp_packet* udp_list_tail[65536] = {0};
static spinlock_t udp_read_lock = 0;

/**
 * @brief Idle callback to check if socket is ready to read.
 *
 * Used by the scheduler to determine if the SOCKREAD statement
 * can resume execution. Returns true if still waiting.
 *
 * @param proc Process being checked
 * @param ptr Pointer containing the socket FD (cast from uintptr_t)
 * @return true if still waiting, false if ready
 */
bool check_sockread_ready(process_t* proc, void* ptr) {
	int64_t fd = (int64_t)(uintptr_t)ptr;
	if (basic_esc()) {
		return false;
	}
	return !sock_ready_to_read((int)fd);
}

/**
 * @brief Process SOCKREAD statement.
 *
 * The SOCKREAD statement yields while waiting for socket data.
 * It repeatedly checks if the socket has data available without
 * consuming it, and yields back to the OS if not ready. Once
 * data is available, the scheduler resumes the statement, which
 * reads and stores the value in a BASIC variable.
 *
 * @param ctx BASIC context
 */
void sockread_statement(struct basic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* var = NULL;

	accept_or_return(SOCKREAD, ctx);

	size_t var_length;
	int64_t fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx, &var_length), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);

	var = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);

	process_t* proc = proc_cur(logical_cpu_id());

	int rv = recv((int)fd, input, MAX_STRINGLEN, false, 100);

	dprintf("sockread recv=%d\n", rv);

	if (rv == 0) {
		// Not ready yet, yield and retry later
		proc_set_idle(proc, check_sockread_ready, (void*)(uintptr_t)fd);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	// Clear idle state if we're resuming from IO
	proc_set_idle(proc, NULL, NULL);

	if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
		proc->state = PROC_RUNNING;
		return;
	}

	input[rv] = 0; // Null-terminate string

	switch (var[var_length - 1]) {
		case '$':
			basic_set_string_variable(var, input, ctx, false, false);
			break;
		case '#': {
			double f = 0;
			atof(input, &f);
			basic_set_double_variable(var, f, ctx, false, false);
			break;
		}
		default:
			basic_set_int_variable(var, atoll(input, 10), ctx, false, false);
			break;
	}

	accept_or_return(NEWLINE, ctx);
	proc->state = PROC_RUNNING;
}

void sockbinread_statement(struct basic_ctx* ctx)
{
	accept_or_return(SOCKBINREAD, ctx);
	size_t var_length;
	int64_t fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx, &var_length), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t address = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t length = expr(ctx);
	if (length && !address_valid_write(address, length)) {
		tokenizer_error_printf(ctx, "Invalid address: %016lx", (uint64_t)address);
		return;
	}

	process_t* proc = proc_cur(logical_cpu_id());

	int rv = recv((int)fd, address, length, false, 100);

	if (rv == 0) {
		// Not ready yet, yield and retry later
		proc_set_idle(proc, check_sockread_ready, (void*)(uintptr_t)fd);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	// Clear idle state if we're resuming from IO
	proc_set_idle(proc, NULL, NULL);

	if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
		proc->state = PROC_RUNNING;
		return;
	}

	accept_or_return(NEWLINE, ctx);
	proc->state = PROC_RUNNING;
}

void connect_statement(struct basic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* fd_var = NULL, *ip = NULL;
	int64_t port;
	size_t var_length;

	accept_or_return(CONNECT, ctx);
	fd_var = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	ip = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	port = expr(ctx);

	int rv = connect(str_to_ip(ip), port, 0, true);

	if (rv >= 0) {
		*(input + rv) = 0;
		switch (fd_var[var_length - 1]) {
			case '$':
				tokenizer_error_print(ctx, "Can't store socket descriptor in STRING");
			break;
			case '#':
				tokenizer_error_print(ctx, "Cannot store socket descriptor in REAL");
			break;
			default:
				basic_set_int_variable(fd_var, rv, ctx, false, false);
			break;
		}

		accept_or_return(NEWLINE, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

void sockclose_statement(struct basic_ctx* ctx)
{
	const char* fd_var = NULL;
	size_t var_length;

	accept_or_return(SOCKCLOSE, ctx);
	fd_var = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);

	int rv = closesocket(basic_get_numeric_int_variable(fd_var, ctx));
	if (rv == 0) {
		// Clear variable to -1
		basic_set_int_variable(fd_var, -1, ctx, false, false);
		accept_or_return(NEWLINE, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

int64_t basic_socklisten(struct basic_ctx* ctx) {
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* ip = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t port = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t backlog = intval;
	PARAMS_END("SOCKLISTEN",-1);
	if (port < 1 || port > UINT16_MAX - 1) {
		tokenizer_error_print(ctx, "Invalid port for LISTEN");
		return -1;
	}
	int rv = tcp_listen(str_to_ip(ip), port, backlog);
	if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
		return -1;
	}
	return rv;
}

int64_t basic_sockaccept(struct basic_ctx* ctx) {
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t server = intval;
	PARAMS_END("SOCKACCEPT",-1);
	int rv = tcp_accept(server);
	if (rv == TCP_ERROR_WOULD_BLOCK) {
		/* This is an expected, handled error */
		return -1;
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
		return -1;
	}
	return rv;
}

char* basic_insocket(struct basic_ctx* ctx) {
	uint8_t input[2] = { 0, 0 };
	
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t fd = intval;
	PARAMS_END("INSOCKET$","");

	if (fd < 0) {
		tokenizer_error_print(ctx, "Invalid socket descriptor");
		return "";
	}

	int rv = recv(fd, input, 1, false, 0);

	if (rv > 0) {
		input[1] = 0;
		return gc_strdup(ctx, (const char*)input);
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
	} else {
		__asm__ volatile("hlt");
	}
	return "";
}

int64_t basic_sockstatus(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t fd = intval;
	PARAMS_END("SOCKSTATUS", 0);

	if (fd < 0) {
		return 0;
	}

	return is_connected(fd);
}

char* basic_netinfo(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("NETINFO$","");
	char ip[16] = { 0 };
	if (!stricmp(strval, "ip")) {
		unsigned char raw[4];
		if (gethostaddr(raw)) {
			get_ip_str(ip, (uint8_t*)&raw);
			return gc_strdup(ctx, ip);
		}
		return gc_strdup(ctx, "0.0.0.0");
	}
	if (!stricmp(strval, "gw")) {
		uint32_t raw = getgatewayaddr();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ctx, ip);
	}
	if (!stricmp(strval, "mask")) {
		uint32_t raw = getnetmask();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ctx, ip);
	}
	if (!stricmp(strval, "dns")) {
		uint32_t raw = getdnsaddr();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ctx, ip);
	}
	return gc_strdup(ctx, "0.0.0.0");
}

char* basic_dns(struct basic_ctx* ctx) {
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("DNS$","");
	char ip[16] = { 0 };
	uint32_t addr = dns_lookup_host(getdnsaddr(), strval, 2000);
	get_ip_str(ip, (uint8_t*)&addr);
	return gc_strdup(ctx, ip);
}

void sockwrite_statement(struct basic_ctx* ctx) {
	accept_or_return(SOCKWRITE, ctx);
	size_t var_length;
	int fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx, &var_length), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		send(fd, out, strlen(out));
	}
}

void sockbinwrite_statement(struct basic_ctx* ctx) {
	accept_or_return(SOCKBINWRITE, ctx);
	size_t var_length;
	int fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx, &var_length), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	uint64_t buffer_pointer = expr(ctx);
	accept_or_return(COMMA, ctx);
	int length = expr(ctx);
	if (length && !address_valid_read(buffer_pointer, length)) {
		tokenizer_error_printf(ctx, "Invalid address: %016lx", buffer_pointer);
		return;
	}

	if (buffer_pointer && length) {
		send(fd, buffer_pointer, length);
	}
}
static void basic_udp_handle_packet(uint32_t src_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t length, void* opaque) {
	basic_ctx* ctx = (basic_ctx*)opaque;
	if (!opaque) {
		return;
	}
	// TODO: Protect this against concurrent access
	queued_udp_packet* packet = buddy_malloc(ctx->allocator, sizeof(queued_udp_packet));
	if (!packet) {
		return;
	}
	char ip[MAX_STRINGLEN];
	src_ip = ntohl(src_ip);
	get_ip_str(ip, (uint8_t*)&src_ip);
	packet->length = length;
	packet->data = buddy_strdup(ctx->allocator, data);
	packet->ip = buddy_strdup(ctx->allocator, ip);
	packet->source_port = src_port;
	packet->next = NULL;
	uint64_t flags;
	lock_spinlock_irq(&udp_read_lock, &flags);
	packet->prev = (struct queued_udp_packet*)udp_list_tail[dst_port];
	if (udp_list_tail[dst_port] == NULL) {
		udp_list_tail[dst_port] = packet;
		udp_packets[dst_port] = packet;
	} else {
		udp_list_tail[dst_port]->next = (struct queued_udp_packet*)packet;
		udp_list_tail[dst_port] = packet;
	}
	unlock_spinlock_irq(&udp_read_lock, flags);
}

void udpwrite_statement(struct basic_ctx* ctx) {
	accept_or_return(UDPWRITE, ctx);
	const char* dest_ip = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t source_port = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t dest_port = expr(ctx);
	accept_or_return(COMMA, ctx);
	const char* data = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (source_port > 65535 || source_port < 0 || dest_port > 65535 || dest_port < 0) {
		tokenizer_error_print(ctx, "Invalid UDP port number");
	}
	size_t len = strlen(data);
	if (len == 0 || len > 65530) {
		tokenizer_error_print(ctx, "Invalid UDP packet length");
	}
	uint32_t dest = htonl(str_to_ip(dest_ip));
	udp_send_packet((uint8_t*)&dest, source_port, dest_port, (void*)data, len + 1); // including the NULL terminator
}

void udpbind_statement(struct basic_ctx* ctx) {
	accept_or_return(UDPBIND, ctx);
	const char* bind_ip = str_expr(ctx);
	(void)bind_ip;
	accept_or_return(COMMA, ctx);
	int64_t port = expr(ctx);
	if (port > 65535 || port < 0) {
		tokenizer_error_print(ctx, "Invalid UDP port number");
	}
	accept_or_return(NEWLINE, ctx);
	udp_register_daemon(port, &basic_udp_handle_packet, ctx);
}

void udpunbind_statement(struct basic_ctx* ctx) {
	accept_or_return(UDPUNBIND, ctx);
	const char* bind_ip = str_expr(ctx);
	(void)bind_ip;
	accept_or_return(COMMA, ctx);
	int64_t port = expr(ctx);
	if (port > 65535 || port < 0) {
		tokenizer_error_print(ctx, "Invalid UDP port number");
	}
	accept_or_return(NEWLINE, ctx);
	udp_unregister_daemon(port, &basic_udp_handle_packet);
}

int64_t basic_udplastsourceport(struct basic_ctx* ctx) {
	return ctx->last_packet.source_port;
}

char* basic_udplastip(struct basic_ctx* ctx) {
	return ctx->last_packet.ip ? (char*)ctx->last_packet.ip : "";
}

char* basic_udpread(struct basic_ctx* ctx) {
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("UDPREAD$","");
	int64_t port = intval;
	if (port > 65535 || port < 0) {
		tokenizer_error_print(ctx, "Invalid UDP port number");
	}
	if (ctx->last_packet.ip) {
		buddy_free(ctx->allocator, ctx->last_packet.ip);
	}
	if (ctx->last_packet.data) {
		buddy_free(ctx->allocator, ctx->last_packet.data);
	}
	memset(&ctx->last_packet, 0, sizeof(ctx->last_packet));
	uint64_t flags;
	lock_spinlock_irq(&udp_read_lock, &flags);
	queued_udp_packet* queue = udp_packets[port];
	if (queue) {
		ctx->last_packet = *queue;
		if (queue == udp_list_tail[port]) {
			/* This packet is the tail packet */
			udp_list_tail[port] = (queued_udp_packet *)udp_list_tail[port]->prev;
			if (udp_list_tail[port]) {
				udp_list_tail[port]->next = NULL;
			}
		}
		udp_packets[port] = (queued_udp_packet *)queue->next;
		if (udp_packets[port]) {
			udp_packets[port]->prev = NULL;
		}
		buddy_free(ctx->allocator, queue);
	} else {
		ctx->last_packet.ip = buddy_strdup(ctx->allocator, "0.0.0.0");
		ctx->last_packet.data = buddy_strdup(ctx->allocator, "");
		ctx->last_packet.length = 0;
		ctx->last_packet.source_port = 0;
	}

	unlock_spinlock_irq(&udp_read_lock, flags);
	return ctx->last_packet.data ? (char*)ctx->last_packet.data : "";
}

/**
 * @brief Idle callback to check if a socket has fully drained its send path.
 *
 * Used by the scheduler to resume SOCKFLUSH when the condition becomes true.
 * Returns true if still waiting (i.e., NOT drained yet).
 */
static bool check_sockflush_ready(process_t* proc, void* ptr) {
	(void)proc;
	int64_t fd = (int64_t)(uintptr_t)ptr;
	if (basic_esc()) {
		return false;
	}
	return !sock_sent(fd);
}

void sockflush_statement(struct basic_ctx* ctx)
{
	size_t var_length;
	accept_or_return(SOCKFLUSH, ctx);
	int64_t fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx, &var_length), ctx);
	accept_or_return(VARIABLE, ctx);

	process_t* proc = proc_cur(logical_cpu_id());

	if (!sock_sent((int)fd)) {
		/* Not drained yet: park this process and retry the same line later */
		proc_set_idle(proc, check_sockflush_ready, (void*)(uintptr_t)fd);
		jump_linenum(ctx->current_linenum, ctx);
		proc->state = PROC_IO_BOUND;
		return;
	}

	/* Drained now: clear idle state and advance */
	proc_set_idle(proc, NULL, NULL);
	accept_or_return(NEWLINE, ctx);
	proc->state = PROC_RUNNING;
}
