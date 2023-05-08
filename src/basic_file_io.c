#include <kernel.h>

const char* make_full_path(const char* relative)
{
	if (*relative == '/') {
		dprintf("make_full_path %s -> %s\n", relative, relative);
		return relative;
	}

	const char* csd = proc_cur()->csd;
	char qualified_path[MAX_STRINGLEN];

	if (*relative == 0) {
		dprintf("make_full_path %s -> %s\n", relative, csd);
		return csd;
	}

	if (*csd == '/' && *(csd+1) == 0) {
		snprintf(qualified_path, MAX_STRINGLEN, "%s%s", csd, relative);
		dprintf("make_full_path %s -> %s\n", relative, qualified_path);
		return gc_strdup(qualified_path);
	}
	snprintf(qualified_path, MAX_STRINGLEN, "%s/%s", csd, relative);
	dprintf("make_full_path %s -> %s\n", relative, qualified_path);
	return gc_strdup(qualified_path);
}

char* basic_readstring(struct basic_ctx* ctx)
{
	char* res = kmalloc(MAX_STRINGLEN);
	int ofs = 0;
	*res = 0;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ$", "");
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

int64_t basic_read(struct basic_ctx* ctx)
{
	char res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ", 0);
	if (_read(intval, &res, 1) != 1) {
		tokenizer_error_print(ctx, "Error reading from file");
	}
	return res;
}

void openin_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENIN is a function");
}

void openup_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENUP is a function");
}

void openout_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENOUT is a function");
}

void read_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "READ is a function");
}

void close_statement(struct basic_ctx* ctx)
{
	accept_or_return(CLOSE, ctx);
	_close(expr(ctx));
	accept_or_return(NEWLINE, ctx);
}

void eof_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "EOF is a function");
}

int64_t basic_open_func(struct basic_ctx* ctx, int oflag)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("OPEN", 0);
	const char* file = make_full_path(strval);
	if (fs_is_directory(file)) {
		tokenizer_error_print(ctx, "Not a file");
		return 0;
	}
	int fd = _open(file, oflag);
	dprintf("basic_open_func(\"%s\") = %d\n", strval, fd);
	return fd;
}

int64_t basic_openin(struct basic_ctx* ctx)
{
	return basic_open_func(ctx, _O_RDONLY);
}

int64_t basic_openout(struct basic_ctx* ctx)
{
	return basic_open_func(ctx, _O_WRONLY);
}

int64_t basic_openup(struct basic_ctx* ctx)
{
	return basic_open_func(ctx, _O_RDWR);
}

int64_t basic_getnamecount(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("GETNAMECOUNT", 0);
	const char* dir = make_full_path(strval);
	if (!fs_is_directory(dir)) {
		tokenizer_error_print(ctx, "Not a directory");
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(dir);
	int count = 0;
	while (fsl) {
		fsl = fsl->next;
		count++;
	}
	return count;
}

char* basic_getname(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETNAME$", "");
	const char* dir = make_full_path(strval);
	if (!fs_is_directory(dir)) {
		tokenizer_error_print(ctx, "Not a directory");
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(dir);
	int count = 0;
	while (fsl) {
		if (count++ == intval) {
			return gc_strdup(fsl->filename);
		}
		fsl = fsl->next;
	}
	return "";
}

char* basic_filetype(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* dir = make_full_path(strval);
	PARAMS_END("FILETYPE$", "");
	return fs_is_directory(dir) ? "directory" : "file";
}

int64_t basic_getsize(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETSIZE", 0);
	const char* dir = make_full_path(strval);
	fs_directory_entry_t* fsl = fs_get_items(dir);
	int count = 0;
	while (fsl) {
		if (count++ == intval) {
			return fsl->size;
		}
		fsl = fsl->next;
	}
	return 0;
}

int64_t basic_eof(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("EOF", 0);
	return _eof(intval);
}

void mkdir_statement(struct basic_ctx* ctx)
{
	accept_or_return(MKDIR, ctx);
	const char* name = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	const char* dir = make_full_path(name);
	if (!fs_create_directory(dir)) {
		tokenizer_error_print(ctx, "Unable to create directory");
	}
}

void mount_statement(struct basic_ctx* ctx)
{
	accept_or_return(MOUNT, ctx);
	const char* path = make_full_path(str_expr(ctx));
	accept_or_return(COMMA, ctx);
	const char* device = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	const char* fs_type = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	filesystem_mount(path, device, fs_type);
}

void rmdir_statement(struct basic_ctx* ctx)
{
	accept_or_return(RMDIR, ctx);
	const char* name = make_full_path(str_expr(ctx));
	accept_or_return(NEWLINE, ctx);
	if (!fs_delete_directory(name)) {
		tokenizer_error_print(ctx, "Unable to delete directory");
	}
}


void delete_statement(struct basic_ctx* ctx)
{
	accept_or_return(DELETE, ctx);
	const char* name = make_full_path(str_expr(ctx));
	accept_or_return(NEWLINE, ctx);
	if (!fs_delete_file(name)) {
		tokenizer_error_print(ctx, "Unable to delete file");
	}
}


void write_statement(struct basic_ctx* ctx)
{
	int fd = -1;

	accept_or_return(WRITE, ctx);
	fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	char* out = printable_syntax(ctx);
	if (out) {
		if (_write(fd, out, strlen(out)) == -1) {
			tokenizer_error_print(ctx, "Error writing to file");
		}
	}
}

