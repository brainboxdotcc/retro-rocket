/**
 * @file basic/file_io.c
 * @brief BASIC file functions
 */
#include <kernel.h>
#include "installer.h"

const char* make_full_path(struct basic_ctx* ctx, const char* relative)
{
	uint8_t cpu = logical_cpu_id();
	if (*relative == '/') {
		return relative;
	}

	const char* csd = proc_cur(cpu)->csd;
	char qualified_path[MAX_STRINGLEN];

	if (*relative == 0) {
		return csd;
	}

	if (*csd == '/' && *(csd+1) == 0) {
		snprintf(qualified_path, MAX_STRINGLEN, "%s%s", csd, relative);
		return gc_strdup(ctx, qualified_path);
	}
	snprintf(qualified_path, MAX_STRINGLEN, "%s/%s", csd, relative);
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
	char* ret = (char*)gc_strdup(ctx, res);
	buddy_free(ctx->allocator, res);
	return ret;
}

int64_t basic_read(struct basic_ctx* ctx)
{
	uint8_t res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ", 0);
	if (_read(intval, &res, 1) != 1) {
		tokenizer_error_printf(ctx, "Error reading from file: %s", fs_strerror(fs_get_error()));
	}
	return res;
}

void close_statement(struct basic_ctx* ctx)
{
	accept_or_return(CLOSE, ctx);
	if (_close(expr(ctx)) < 0) {
		tokenizer_error_printf(ctx, "Error closing file: %s", fs_strerror(fs_get_error()));
	}
	accept_or_return(NEWLINE, ctx);
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
	if (!fsl && fs_get_error() != FS_ERR_NO_ERROR) {
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
	if (!fsl && fs_get_error() != FS_ERR_NO_ERROR) {
		tokenizer_error_printf(ctx, "Error retrieving directory items: %s", fs_strerror(fs_get_error()));
		return 0;
	}
	int count = 0;
	while (fsl) {
		if (count++ == intval) {
			return (char*)gc_strdup(ctx, fsl->filename);
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

int64_t basic_filesize(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("FILESIZE", 0);
	fs_directory_entry_t* file = fs_get_file_info(strval);
	if (file) {
		return file->size;
	}
	tokenizer_error_printf(ctx, "Error retrieving size: %s", fs_strerror(fs_get_error()));
	return 0;
}

int64_t basic_getsize(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETSIZE", 0);
	const char* dir = make_full_path(ctx, strval);
	fs_directory_entry_t* fsl = fs_get_items(dir);
	if (!fsl && fs_get_error() != FS_ERR_NO_ERROR) {
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

int64_t basic_tell(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("TELL", 0);
	int64_t where = _tell(intval);
	if (where == -1) {
		tokenizer_error_printf(ctx, "TELL: %s", fs_strerror(fs_get_error()));
		return 0;
	}
	return where;
}

void seek_statement(struct basic_ctx* ctx)
{
	accept_or_return(SEEK, ctx);
	int64_t fd = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t pos = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	if (_lseek(fd, pos, 0) == -1) {
		tokenizer_error_printf(ctx, "SEEK: %s", fs_strerror(fs_get_error()));
		return;
	}
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

void readbinary_statement(struct basic_ctx* ctx)
{
	accept_or_return(BINREAD, ctx);
	int64_t fd = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t buffer = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t size = expr(ctx);
	if (size && !address_valid_write(buffer, size)) {
		tokenizer_error_printf(ctx, "Invalid address: %016lx", (uint64_t)buffer);
		return;
	}
	if (size && _read(fd, buffer, size) == -1) {
		tokenizer_error_printf(ctx, "Error reading from file: %s", fs_strerror(fs_get_error()));
	}
	accept_or_return(NEWLINE, ctx);
}

void writebinary_statement(struct basic_ctx* ctx)
{
	accept_or_return(BINWRITE, ctx);
	int64_t fd = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t buffer = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t size = expr(ctx);
	if (size && !address_valid_read(buffer, size)) {
		tokenizer_error_printf(ctx, "Invalid address: %016lx", (uint64_t)buffer);
		return;
	}
	if (size && _write(fd, buffer, size) == -1) {
		tokenizer_error_printf(ctx, "Error writing to file: %s", fs_strerror(fs_get_error()));
	}
	accept_or_return(NEWLINE, ctx);
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
	size_t var_length;
	fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx, &var_length), ctx);
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
	return (char*)gc_strdup(ctx, rd);
}

char* basic_ramdisk_from_image(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("ADFSIMAGE$","");
	const char* imagefile = make_full_path(ctx, strval);
	fs_directory_entry_t *info = fs_get_file_info(imagefile);
	if (!info || (info->flags & FS_DIRECTORY) != 0) {
		tokenizer_error_printf(ctx, "ADFS/DFS Image %s not found or is a directory", imagefile);
		return "";
	}
	uint8_t* image = kmalloc(info->size);
	if (!image) {
		tokenizer_error_print(ctx, "Out of memory for ADFS/DFS disk image");
		return "";
	}
	if (!fs_read_file(info, 0, info->size, image)) {
		kfree_null(&image);
		tokenizer_error_printf(ctx, "Unable to load ADFS/DFS disk image from %s", imagefile);
		return "";
	}
	const char* rd;
	if (info->size == 640 * 1024) {
		rd = init_ramdisk_from_adfs_image(image, info->size);
	} else {
		rd = init_ramdisk_from_dfs_image(image, info->size);
	}
	if (!rd) {
		kfree_null(&image);
		return "";
	}
	return (char*)gc_strdup(ctx, rd);
}

char* basic_ramdisk_from_size(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t device_size_mb = intval;
	PARAMS_END("EMPTYRAMDISK$","");

	/* A RetroFS device of 512kb is actually usable if you only want literally a couple of files, but we make them have more */
	if (device_size_mb < 2) {
		tokenizer_error_printf(ctx, "Ramdisk too small for RetroFS (minimum 2mb)");
		return "";
	}
	/* Can't allocate bigger than free memory, or bigger than half the total memory */
	if ((uint64_t)device_size_mb * 1024 > get_free_memory() || (uint64_t)device_size_mb * 1024 > get_total_memory() / 2) {
		tokenizer_error_printf(ctx, "Ramdisk too large");
		return "";
	}

	const char* rd = init_ramdisk(device_size_mb * 2048, 512);
	if (!rd) {
		tokenizer_error_printf(ctx, "Failed to initialise ramdisk of %lu sectors", device_size_mb * 2048);
		return "";
	}
	storage_device_t* block_device = find_storage_device(rd);
	if (!prepare_rfs_partition(block_device, false)) {
		tokenizer_error_printf(ctx, "Failed to format ramdisk '%s'", rd);
		return "";
	}
	return (char*)gc_strdup(ctx, rd);
}


char* basic_csd(struct basic_ctx* ctx)
{
	return (char*)gc_strdup(ctx, proc_cur(logical_cpu_id())->csd);
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

