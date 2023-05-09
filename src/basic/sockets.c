/**
 * @file basic/sockets.c
 * @brief BASIC network socket functions
 */
#include <kernel.h>

/**
 * @brief Process SOCKREAD statement.
 * 
 * The SOCKREAD statement will yield while waiting for data, essentially if
 * there is no complete line in the data buffer yet, it will yield back to
 * the OS task loop for other processes to get a turn. Each time the process
 * is entered while waiting for the data to be completed, it will just
 * loop back against the same SOCKREAD statement again until completed.
 * 
 * @param ctx BASIC context
 */
void sockread_statement(struct basic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* var = NULL;
	int fd = -1;

	accept_or_return(SOCKREAD, ctx);
	fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	int rv = recv(fd, input, MAX_STRINGLEN, false, 10);

	if (rv > 0) {
		*(input + rv) = 0;
		switch (var[strlen(var) - 1]) {
			case '$':
				basic_set_string_variable(var, input, ctx, false, false);
			break;
			case '#':
				double f = 0;
				atof(input, &f);
				basic_set_double_variable(var, f, ctx, false, false);
			break;
			default:
				basic_set_int_variable(var, atoll(input, 10), ctx, false, false);
			break;
		}

		accept_or_return(NEWLINE, ctx);
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
	} else {
		jump_linenum(ctx->current_linenum, ctx);
	}
}

void connect_statement(struct basic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* fd_var = NULL, *ip = NULL;
	int64_t port = 0;

	accept_or_return(CONNECT, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	ip = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	port = expr(ctx);

	int rv = connect(str_to_ip(ip), port, 0, true);

	if (rv >= 0) {
		*(input + rv) = 0;
		switch (fd_var[strlen(fd_var) - 1]) {
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

	accept_or_return(SOCKCLOSE, ctx);
	fd_var = tokenizer_variable_name(ctx);
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

char* basic_insocket(struct basic_ctx* ctx)
{
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
		return gc_strdup((const char*)input);
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
			return gc_strdup(ip);
		}
		return gc_strdup("0.0.0.0");
	}
	if (!stricmp(strval, "gw")) {
		uint32_t raw = getgatewayaddr();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ip);
	}
	if (!stricmp(strval, "mask")) {
		uint32_t raw = getnetmask();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ip);
	}
	if (!stricmp(strval, "dns")) {
		uint32_t raw = getdnsaddr();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ip);
	}
	return gc_strdup("0.0.0.0");
}

char* basic_dns(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("DNS$","");
	char ip[16] = { 0 };
	uint32_t addr = dns_lookup_host(getdnsaddr(), strval, 2);
	get_ip_str(ip, (uint8_t*)&addr);
	return gc_strdup(ip);
}

void sockwrite_statement(struct basic_ctx* ctx)
{
	int fd = -1;

	accept_or_return(SOCKWRITE, ctx);
	fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		send(fd, out, strlen(out));
	}
}

