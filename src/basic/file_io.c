/**
 * @file basic/file_io.c
 * @brief BASIC file functions
 */
#include <kernel.h>

const char* make_full_path(struct basic_ctx* ctx, const char* relative)
{
	uint8_t cpu = logical_cpu_id();
	if (*relative == '/') {
		dprintf("make_full_path %s -> %s\n", relative, relative);
		return relative;
	}

	const char* csd = proc_cur(cpu)->csd;
	char qualified_path[MAX_STRINGLEN];

	if (*relative == 0) {
		dprintf("make_full_path %s -> %s\n", relative, csd);
		return csd;
	}

	if (*csd == '/' && *(csd+1) == 0) {
		snprintf(qualified_path, MAX_STRINGLEN, "%s%s", csd, relative);
		dprintf("make_full_path %s -> %s\n", relative, qualified_path);
		return gc_strdup(ctx, qualified_path);
	}
	snprintf(qualified_path, MAX_STRINGLEN, "%s/%s", csd, relative);
	dprintf("make_full_path %s -> %s\n", relative, qualified_path);
	return gc_strdup(ctx, qualified_path);
}

char* basic_readstring(struct basic_ctx* ctx)
{
	int ofs = 0;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ$", "");
	char* res = buddy_malloc(ctx->allocator, MAX_STRINGLEN);
	if (!res) {
		tokenizer_error_print(ctx, "Error allocating string buffer");
		return "";
	}
	*res = 0;
	while (!_eof(intval) && ofs < MAX_STRINGLEN) {
		if (_read(intval, res + ofs, 1) != 1) {
			tokenizer_error_printf(ctx, "Error reading from file: %s", fs_strerror(fs_get_error()));
			buddy_free(ctx->allocator, res);
			return "";
		}
		if (*(res + ofs) == '\n') {
			break;
		} else {
			ofs++;
		}
	}
	*(res+ofs) = 0;
	char* ret = gc_strdup(ctx, res);
	buddy_free(ctx->allocator, res);
	return ret;
}

int64_t basic_read(struct basic_ctx* ctx)
{
	char res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ", 0);
	if (_read(intval, &res, 1) != 1) {
		tokenizer_error_printf(ctx, "Error reading from file: %s", fs_strerror(fs_get_error()));
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

void data_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "DATA statements are not supported in Retro Rocket BASIC. Use files instead.");
}

void restore_statement(struct basic_ctx* ctx)
{
	tokenizer_error_print(ctx, "Nothing to RESTORE. DATA statements are not supported.");
}

void close_statement(struct basic_ctx* ctx)
{
	accept_or_return(CLOSE, ctx);
	if (_close(expr(ctx)) < 0) {
		tokenizer_error_printf(ctx, "Error closing file: %s", fs_strerror(fs_get_error()));
	}
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
	const char* file = make_full_path(ctx, strval);
	if (fs_is_directory(file)) {
		tokenizer_error_printf(ctx, "Not a file: '%s'", file);
		return 0;
	}
	int fd = _open(file, oflag);
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
	const char* dir = make_full_path(ctx, strval);
	if (!fs_is_directory(dir)) {
		tokenizer_error_printf(ctx, "Not a directory: '%s'", dir);
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(dir);
	if (!fsl) {
		tokenizer_error_printf(ctx, "Error retrieving directory items: %s", fs_strerror(fs_get_error()));
		return 0;
	}
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
	const char* dir = make_full_path(ctx, strval);
	if (!fs_is_directory(dir)) {
		tokenizer_error_printf(ctx, "Not a directory: '%s'", dir);
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(dir);
	if (!fsl) {
		tokenizer_error_printf(ctx, "Error retrieving directory items: %s", fs_strerror(fs_get_error()));
		return 0;
	}
	int count = 0;
	while (fsl) {
		if (count++ == intval) {
			return gc_strdup(ctx, fsl->filename);
		}
		fsl = fsl->next;
	}
	return "";
}

char* basic_filetype(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* dir = make_full_path(ctx, strval);
	PARAMS_END("FILETYPE$", "");
	return fs_is_directory(dir) ? "directory" : "file";
}

int64_t basic_getsize(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETSIZE", 0);
	const char* dir = make_full_path(ctx, strval);
	fs_directory_entry_t* fsl = fs_get_items(dir);
	if (!fsl) {
		tokenizer_error_printf(ctx, "Error retrieving directory items: %s", fs_strerror(fs_get_error()));
		return 0;
	}
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
	const char* dir = make_full_path(ctx, name);
	if (!fs_create_directory(dir)) {
		tokenizer_error_printf(ctx, "Unable to create directory '%s': %s", dir, fs_strerror(fs_get_error()));
	}
}

void mount_statement(struct basic_ctx* ctx)
{
	accept_or_return(MOUNT, ctx);
	const char* path = make_full_path(ctx, str_expr(ctx));
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
	const char* name = make_full_path(ctx, str_expr(ctx));
	accept_or_return(NEWLINE, ctx);
	if (!fs_delete_directory(name)) {
		tokenizer_error_printf(ctx, "Unable to delete directory '%s': %s", name, fs_strerror(fs_get_error()));
	}
}


void delete_statement(struct basic_ctx* ctx)
{
	accept_or_return(DELETE, ctx);
	const char* name = make_full_path(ctx, str_expr(ctx));
	accept_or_return(NEWLINE, ctx);
	if (!fs_delete_file(name)) {
		tokenizer_error_printf(ctx, "Unable to delete file '%s': %s", name, fs_strerror(fs_get_error()));
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
			tokenizer_error_printf(ctx, "Error writing to file: %s", fs_strerror(fs_get_error()));
		}
	}
}

char* basic_ramdisk_from_device(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("RAMDISK$","");
	const char* rd = init_ramdisk_from_storage(strval);
	if (!rd) {
		return "";
	}
	return gc_strdup(ctx, rd);
}

char* basic_ramdisk_from_size(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t blocks = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t block_size = intval;
	PARAMS_END("RAMDISK","");
	const char* rd = init_ramdisk(blocks, block_size);
	if (!rd) {
		return "";
	}
	return gc_strdup(ctx, rd);
}


char* basic_csd(struct basic_ctx* ctx)
{
	return gc_strdup(ctx, proc_cur(logical_cpu_id())->csd);
}

void chdir_statement(struct basic_ctx* ctx)
{
	accept_or_return(CHDIR, ctx);
	const char* csd = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	uint8_t cpu = logical_cpu_id();
	process_t* proc = proc_cur(cpu);
	// NOTE: CSD does NOT use the BASIC allocator, VFS needs reference to this
	const char* old = strdup(proc->csd);
	const char* new = proc_set_csd(proc, csd);
	if (new && fs_is_directory(new)) {
		kfree_null(&old);
		return;
	}
	if (!new) {
		tokenizer_error_printf(ctx, "Invalid directory '%s'", csd);
	} else if (!fs_is_directory(new)) {
		tokenizer_error_printf(ctx, "Not a directory '%s'", csd);
	}

	proc_set_csd(proc, old);
	kfree_null(&old);
}

