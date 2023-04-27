#include <kernel.h>

char* ubasic_readstring(struct ubasic_ctx* ctx)
{
	char* res = kmalloc(MAX_STRINGLEN);
	int ofs = 0;
	*res = 0;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ$");
	while (!_eof(intval) && ofs < MAX_STRINGLEN) {
		if (_read(intval, res + ofs, 1) != 1)
			tokenizer_error_print(ctx, "Error reading from file");
		if (*(res + ofs) == '\n')
			break;
		else
			ofs++;
	}
	*(res+ofs) = 0;
	char* ret = gc_strdup(res);
	kfree(res);
	return ret;
}

int64_t ubasic_read(struct ubasic_ctx* ctx)
{
	char res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ");
	if (_read(intval, &res, 1) != 1) {
		tokenizer_error_print(ctx, "Error reading from file");
	}
	return res;
}

void openin_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENIN is a function");
}

void openup_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENUP is a function");
}

void openout_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENOUT is a function");
}

void read_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "READ is a function");
}

void close_statement(struct ubasic_ctx* ctx)
{
	accept(CLOSE, ctx);
	_close(expr(ctx));
	accept(NEWLINE, ctx);
}

void eof_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "EOF is a function");
}

int64_t ubasic_open_func(struct ubasic_ctx* ctx, int oflag)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("OPEN");
	if (fs_is_directory(strval)) {
		tokenizer_error_print(ctx, "Not a file");
		return 0;
	}
	int fd = _open(strval, oflag);
	dprintf("ubasic_open_func: %s returned: %d\n", strval, fd);
	return fd;
}

int64_t ubasic_openin(struct ubasic_ctx* ctx)
{
	return ubasic_open_func(ctx, _O_RDONLY);
}

int64_t ubasic_openout(struct ubasic_ctx* ctx)
{
	return ubasic_open_func(ctx, _O_WRONLY);
}

int64_t ubasic_openup(struct ubasic_ctx* ctx)
{
	return ubasic_open_func(ctx, _O_RDWR);
}

int64_t ubasic_getnamecount(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("GETNAMECOUNT");
	if (!fs_is_directory(strval)) {
		tokenizer_error_print(ctx, "Not a directory");
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl) {
		fsl = fsl->next;
		count++;
	}
	return count;
}

char* ubasic_getname(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETNAME$");
	if (!fs_is_directory(strval)) {
		tokenizer_error_print(ctx, "Not a directory");
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl) {
		if (count++ == intval) {
			return gc_strdup(fsl->filename);
		}
		fsl = fsl->next;
	}
	return "";
}

int64_t ubasic_getsize(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETSIZE");
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl) {
		if (count++ == intval) {
			return fsl->size;
		}
		fsl = fsl->next;
	}
	return 0;
}

int64_t ubasic_eof(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("EOF");
	return _eof(intval);
}

void mkdir_statement(struct ubasic_ctx* ctx)
{
	accept(MKDIR, ctx);
	const char* name = str_expr(ctx);
	accept(NEWLINE, ctx);
	if (!fs_create_directory(name)) {
		tokenizer_error_print(ctx, "Unable to create directory");
	}
}

void mount_statement(struct ubasic_ctx* ctx)
{
	accept(MOUNT, ctx);
	const char* path = str_expr(ctx);
	accept(COMMA, ctx);
	const char* device = str_expr(ctx);
	accept(COMMA, ctx);
	const char* fs_type = str_expr(ctx);
	accept(NEWLINE, ctx);
	filesystem_mount(path, device, fs_type);
}

void rmdir_statement(struct ubasic_ctx* ctx)
{
	accept(RMDIR, ctx);
	const char* name = str_expr(ctx);
	accept(NEWLINE, ctx);
	if (!fs_delete_directory(name)) {
		tokenizer_error_print(ctx, "Unable to delete directory");
	}
}


void delete_statement(struct ubasic_ctx* ctx)
{
	accept(DELETE, ctx);
	const char* name = str_expr(ctx);
	accept(NEWLINE, ctx);
	if (!fs_delete_file(name)) {
		tokenizer_error_print(ctx, "Unable to delete file");
	}
}


void write_statement(struct ubasic_ctx* ctx)
{
	int fd = -1;

	accept(WRITE, ctx);
	fd = ubasic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	char* out = printable_syntax(ctx);
	if (out) {
		_write(fd, out, strlen(out));
	}
}

