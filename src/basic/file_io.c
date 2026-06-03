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
	char qualified_path[MAX_PATH_LEN];

	if (*relative == 0) {
		return csd;
	}

	if (*csd == '/' && *(csd+1) == 0) {
		snprintf(qualified_path, MAX_PATH_LEN, "%s%s", csd, relative);
		return gc_strdup(ctx, qualified_path);
	}
	snprintf(qualified_path, MAX_PATH_LEN, "%s/%s", csd, relative);
	return gc_strdup(ctx, qualified_path);
}

char* basic_readstring(struct basic_ctx* ctx)
{
	size_t ofs = 0;
	size_t cap = MAX_STRINGLEN;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("READ$", "");
	char* res = buddy_malloc(ctx->allocator, cap);
	if (!res) {
		tokenizer_error_print(ctx, "Error allocating string buffer");
		return "";
	}
	*res = 0;
	while (!_eof(intval)) {
		if (ofs + 1 >= cap) {
			cap *= 2;
			char* new_res = buddy_realloc(ctx->allocator, res, cap);
			if (!new_res) {
				tokenizer_error_print(ctx, "Error allocating string buffer");
				buddy_free(ctx->allocator, res);
				return "";
			}
			res = new_res;
		}

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
	*(res + ofs) = 0;
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

int64_t basic_is_program(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* fileinfo = make_full_path(ctx, strval);
	PARAMS_END("ISPROGRAM", 0);
	fs_directory_entry_t* file = fs_get_file_info(fileinfo);
	if (!file) {
		tokenizer_error_printf(ctx, "Error retrieving file information: %s", fs_strerror(fs_get_error()));
		return 0;
	}
	/* We only care about the first two kilobytes */
	size_t size = file->size;
	size = MIN(size, 2048);
	const char* data = buddy_malloc(ctx->allocator, size);
	if (!data) {
		tokenizer_error_printf(ctx, "Out of memory reading file: %s", fs_strerror(fs_get_error()));
		return 0;
	}
	if (!fs_read_file(file, 0, size, (unsigned char*)data)) {
		tokenizer_error_printf(ctx, "Error reading file: %s", fs_strerror(fs_get_error()));
		return 0;
	}
	bool is_program = is_basic(data, size);
	buddy_free(ctx->allocator, data);
	return is_program;
}

int64_t basic_filesize(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("FILESIZE", 0);
	const char* fileinfo = make_full_path(ctx, strval);
	fs_directory_entry_t* file = fs_get_file_info(fileinfo);
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
	const char* name = str_expr(ctx, NULL);
	accept_or_return(NEWLINE, ctx);
	const char* dir = make_full_path(ctx, name);
	if (!fs_create_directory(dir)) {
		tokenizer_error_printf(ctx, "Unable to create directory '%s': %s", dir, fs_strerror(fs_get_error()));
	}
}

void mount_statement(struct basic_ctx* ctx) {
	accept_or_return(MOUNT, ctx);
	const char* path = make_full_path(ctx, str_expr(ctx, NULL));
	accept_or_return(COMMA, ctx);
	const char* device = str_expr(ctx, NULL);
	accept_or_return(COMMA, ctx);
	const char* fs_type = str_expr(ctx, NULL);
	accept_or_return(NEWLINE, ctx);
	int partition = PARTITION_FIRST_MATCH;
	char device_name[16];
	strlcpy(device_name, device, sizeof(device_name));
	char* comma = strchr(device_name, ',');
	if (comma != NULL) {
		*comma = 0;
		char* endptr;
		uint64_t value = strtoul(comma + 1, &endptr, 10);
		if (*endptr || value > 255) {
			tokenizer_error_print(ctx, "Invalid partition number");
			return;
		}
		partition = value;
	}
	filesystem_mount(path, device_name, fs_type, partition);
}

void rmdir_statement(struct basic_ctx* ctx)
{
	accept_or_return(RMDIR, ctx);
	const char* name = make_full_path(ctx, str_expr(ctx, NULL));
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
	if (size && is_restricted_len(ctx, "MEMORY", 6) && !memory_grants_contains(&ctx->memory_grants, buffer, size)) {
		tokenizer_error_printf(ctx, "Bad address &%016lx", buffer);
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
	if (size && is_restricted_len(ctx, "MEMORY", 6) && !memory_grants_contains(&ctx->memory_grants, buffer, size)) {
		tokenizer_error_printf(ctx, "Bad address &%016lx", buffer);
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
	const char* name = make_full_path(ctx, str_expr(ctx, NULL));
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
	size_t len;
	char* out = printable_syntax(ctx, &len);
	if (out) {
		if (_write(fd, out, len) == -1) {
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
	const char* csd = str_expr(ctx, NULL);
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

int64_t basic_verify(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* file = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* sig = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* cert = strval;
	PARAMS_END("VERIFY", 0);

	const char* full_file = make_full_path(ctx, file);
	const char* full_sig = make_full_path(ctx, sig);
	const char* full_cert = make_full_path(ctx, cert);

	fs_directory_entry_t* file_entry = fs_get_file_info(full_file);

	if (!file_entry) {
		tokenizer_error_printf(ctx, "Error retrieving file information: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	fs_directory_entry_t* sig_entry = fs_get_file_info(full_sig);

	if (!sig_entry) {
		tokenizer_error_printf(ctx, "Error retrieving signature information: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	fs_directory_entry_t* cert_entry = fs_get_file_info(full_cert);

	if (!cert_entry) {
		tokenizer_error_printf(ctx, "Error retrieving certificate information: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	fs_directory_entry_t* root_entry = fs_get_file_info("/system/ssl/package_root_ca.pem");

	if (!root_entry) {
		tokenizer_error_printf(ctx, "Error retrieving root certificate information: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	size_t file_size = file_entry->size;
	size_t sig_size = sig_entry->size;
	size_t cert_size = cert_entry->size;
	size_t root_size = root_entry->size;

	uint8_t* file_data = buddy_malloc(ctx->allocator, file_size);

	if (!file_data) {
		tokenizer_error_printf(ctx, "Out of memory reading file");
		return 0;
	}

	uint8_t* sig_data = buddy_malloc(ctx->allocator, sig_size);

	if (!sig_data) {
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Out of memory reading signature");
		return 0;
	}

	uint8_t* cert_data = buddy_malloc(ctx->allocator, cert_size);

	if (!cert_data) {
		buddy_free(ctx->allocator, sig_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Out of memory reading certificate");
		return 0;
	}

	uint8_t* root_data = buddy_malloc(ctx->allocator, root_size);

	if (!root_data) {
		buddy_free(ctx->allocator, cert_data);
		buddy_free(ctx->allocator, sig_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Out of memory reading root certificate");
		return 0;
	}

	if (!fs_read_file(file_entry, 0, file_size, file_data)) {
		buddy_free(ctx->allocator, root_data);
		buddy_free(ctx->allocator, cert_data);
		buddy_free(ctx->allocator, sig_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error reading file: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	if (!fs_read_file(sig_entry, 0, sig_size, sig_data)) {
		buddy_free(ctx->allocator, root_data);
		buddy_free(ctx->allocator, cert_data);
		buddy_free(ctx->allocator, sig_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error reading signature: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	if (!fs_read_file(cert_entry, 0, cert_size, cert_data)) {
		buddy_free(ctx->allocator, root_data);
		buddy_free(ctx->allocator, cert_data);
		buddy_free(ctx->allocator, sig_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error reading certificate: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	if (!fs_read_file(root_entry, 0, root_size, root_data)) {
		buddy_free(ctx->allocator, root_data);
		buddy_free(ctx->allocator, cert_data);
		buddy_free(ctx->allocator, sig_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error reading root certificate: %s", fs_strerror(fs_get_error()));
		return 0;
	}

	int64_t valid = verify_package(file_data, file_size, sig_data, sig_size, cert_data, cert_size, root_data, root_size);

	buddy_free(ctx->allocator, root_data);
	buddy_free(ctx->allocator, cert_data);
	buddy_free(ctx->allocator, sig_data);
	buddy_free(ctx->allocator, file_data);

	return valid;
}

char* basic_sign(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* file = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* key = strval;
	PARAMS_END("SIGN$", "");

	const char* full_file = make_full_path(ctx, file);
	const char* full_key = make_full_path(ctx, key);

	fs_directory_entry_t* file_entry = fs_get_file_info(full_file);

	if (!file_entry) {
		tokenizer_error_printf(ctx, "Error retrieving file information: %s", fs_strerror(fs_get_error()));
		return "";
	}

	fs_directory_entry_t* key_entry = fs_get_file_info(full_key);

	if (!key_entry) {
		tokenizer_error_printf(ctx, "Error retrieving private key information: %s", fs_strerror(fs_get_error()));
		return "";
	}

	size_t file_size = file_entry->size;
	size_t key_size = key_entry->size;

	uint8_t* file_data = buddy_malloc(ctx->allocator, file_size);

	if (!file_data) {
		tokenizer_error_printf(ctx, "Out of memory reading file");
		return "";
	}

	uint8_t* key_data = buddy_malloc(ctx->allocator, key_size);

	if (!key_data) {
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Out of memory reading private key");
		return "";
	}

	if (!fs_read_file(file_entry, 0, file_size, file_data)) {
		buddy_free(ctx->allocator, key_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error reading file: %s", fs_strerror(fs_get_error()));
		return "";
	}

	if (!fs_read_file(key_entry, 0, key_size, key_data)) {
		buddy_free(ctx->allocator, key_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error reading private key: %s", fs_strerror(fs_get_error()));
		return "";
	}

	uint8_t sig[256];
	size_t sig_size = 0;

	if (!sign_package(file_data, file_size, key_data, key_size, sig, &sig_size)) {
		buddy_free(ctx->allocator, key_data);
		buddy_free(ctx->allocator, file_data);
		tokenizer_error_printf(ctx, "Error signing package");
		return "";
	}

	sig[sig_size] = 0;
	STRING_ESCAPE_INPLACE(sig, sig_size, sizeof(sig));

	buddy_free(ctx->allocator, key_data);
	buddy_free(ctx->allocator, file_data);

	return (char*)gc_strdup(ctx, (const char*)sig);
}

int64_t basic_compress(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	uint8_t* input = (uint8_t*)intval;

	PARAMS_GET_ITEM(BIP_INT);
	size_t input_len = intval;

	PARAMS_GET_ITEM(BIP_INT);
	uint8_t* output = (uint8_t*)intval;

	PARAMS_GET_ITEM(BIP_INT);
	size_t output_max = intval;

	PARAMS_GET_ITEM(BIP_INT);
	int level = intval;

	PARAMS_END("COMPRESS", 0);

	uint8_t* compressed = NULL;
	uint32_t compressed_size = 0;

	if (!compress_gzip(input, input_len, &compressed, &compressed_size, level)) {
		tokenizer_error_printf(ctx, "Compression failed");
		return 0;
	}

	if (compressed_size > output_max) {
		kfree(compressed);
		tokenizer_error_printf(ctx, "Output buffer too small");
		return 0;
	}

	memcpy(output, compressed, compressed_size);

	kfree(compressed);

	return compressed_size;
}

int64_t basic_decompress(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	uint8_t* input = (uint8_t*)intval;

	PARAMS_GET_ITEM(BIP_INT);
	size_t input_len = intval;

	PARAMS_GET_ITEM(BIP_INT);
	uint8_t* output = (uint8_t*)intval;

	PARAMS_GET_ITEM(BIP_INT);
	size_t output_max = intval;

	PARAMS_END("DECOMPRESS", 0);

	uint8_t* decompressed = NULL;
	uint32_t decompressed_size = 0;

	if (!decompress_gzip(input, input_len, &decompressed, &decompressed_size)) {
		tokenizer_error_printf(ctx, "Decompression failed");
		return 0;
	}

	if (decompressed_size > output_max) {
		kfree(decompressed);
		tokenizer_error_printf(ctx, "Output buffer too small");
		return 0;
	}

	memcpy(output, decompressed, decompressed_size);

	kfree(decompressed);

	return decompressed_size;
}

typedef struct volume_name_t {
	const char* name;
	struct volume_name_t* next;
} volume_name_t;

typedef struct volume_details_t {
	struct basic_ctx* ctx;
	struct volume_name_t* list;
	struct volume_name_t* tail;
	size_t count;
} volume_details_t;

void get_device_callback(int8_t index, bool matched, const char* description, void* opaque)
{
	volume_details_t* details = opaque;
	volume_name_t* name = buddy_malloc(details->ctx->allocator, sizeof(volume_name_t));
	name->name = buddy_strdup(details->ctx->allocator, description);
	name->next = NULL;
	if (details->tail) {
		details->tail->next = name;
	} else {
		details->list = name;
	}
	details->tail = name;
	details->count++;
}

volume_details_t get_device_volumes(struct basic_ctx* ctx, const char* device_name)
{
	text_guid_t found_guid;
	uint8_t pid;
	uint64_t start, len;
	volume_details_t details = { .ctx = ctx, .list = NULL, .tail = NULL, .count = 0 };
	volume_enumerator_t volume_enumerator = { .fn = get_device_callback, .opaque = &details };
	(void)find_partition_of_type(device_name, UINT8_MAX, found_guid, "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF", &pid, &start, &len, 0, UINT8_MAX, &volume_enumerator);
	return details;
}

void free_device_volumes(volume_details_t* details)
{
	volume_name_t* curr = details->list;
	while (curr) {
		volume_name_t* next = curr->next;
		buddy_free(details->ctx->allocator, (void*)curr->name);
		buddy_free(details->ctx->allocator, curr);
		curr = next;
	}
	details->list = NULL;
	details->tail = NULL;
}

int64_t basic_volcount(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* device_name = strval;
	PARAMS_END("VOLCOUNT", 0);
	if (!find_storage_device(device_name)) {
		tokenizer_error_printf(ctx, "No such storage device: %s", device_name);
		return 0;
	}
	volume_details_t details = get_device_volumes(ctx, device_name);
	free_device_volumes(&details);
	return details.count;
}

const char* get_device_volume_by_index(const volume_details_t* details, size_t index)
{
	if (index >= details->count) {
		return "";
	}
	volume_name_t* curr = details->list;
	while (index && curr) {
		curr = curr->next;
		index--;
	}
	return curr ? curr->name : "";
}

char* basic_voldesc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* device_name = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t index = intval;
	PARAMS_END("VOL$", "");
	if (!find_storage_device(device_name)) {
		tokenizer_error_printf(ctx, "No such storage device: %s", device_name);
		return "";
	}
	volume_details_t details = get_device_volumes(ctx, device_name);
	const char* desc = gc_strdup(ctx, get_device_volume_by_index(&details, index));
	free_device_volumes(&details);
	return (char*)desc;
}
