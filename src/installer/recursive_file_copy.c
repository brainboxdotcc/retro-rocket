/**
 * @file recursive_file_copy.c
 * @author Craig Edwards
 * @brief Recursive directory structure copier for installer
 * @copyright Copyright (c) 2012-2025
 */
#include <installer.h>

static int progress = 0, block_progress = 0;

bool copy_file(const char* source, const char* destination) {
	fs_directory_entry_t* info = fs_get_file_info(source);
	if (info && !(info->flags & FS_DIRECTORY) && info->size > 0) {
		void *file_contents = kmalloc(info->size);
		if (!file_contents) {
			return false;
		}
		bool read_success = fs_read_file(info, 0, info->size, file_contents);
		if (!read_success) {
			error_page("Error reading '%s' (%s)\n", source, fs_strerror(fs_get_error()));
			kfree_null(&file_contents);
			return false;
		}
		int fh = _open(destination, _O_APPEND);
		if (fh < 0) {
			error_page("Error opening '%s' for writing (%s)\n", destination, fs_strerror(fs_get_error()));
			kfree_null(&file_contents);
			return false;
		}
		if (_write(fh, file_contents, info->size) == -1) {
			error_page("Error writing to '%s' (%s)\n", destination, fs_strerror(fs_get_error()));
			kfree_null(&file_contents);
			return false;
		}
		_close(fh);
		kfree_null(&file_contents);

		char message[MAX_STRINGLEN];
		snprintf(message, MAX_STRINGLEN, "Copying '%s' -> '%s' (%lu bytes)\n", source, destination, info->size);
		display_progress(message, progress + ((block_progress++) / 3));

		return true;
	}
	if (!info) {
		error_page("Could not get info for '%s'\n", source);
	}
	return false;
}

bool copy_directory(const char* source, const char* destination) {
	if (!fs_create_directory(destination) && fs_get_error() != FS_ERR_DIRECTORY_EXISTS) {
		error_page("Error creating directory '%s' (%s)\n", destination, fs_strerror(fs_get_error()));
		return false;
	}
	fs_directory_entry_t* fsl = fs_get_items(source);
	while (fsl) {
		if (fsl->flags & FS_DIRECTORY) {
			char subdirectory[MAX_STRINGLEN], new_destination[MAX_STRINGLEN];
			snprintf(subdirectory, MAX_STRINGLEN - 1, "%s/%s", source, fsl->filename);
			snprintf(new_destination, MAX_STRINGLEN - 1, "%s/%s", destination, fsl->filename);
			if (!copy_directory(subdirectory, new_destination)) {
				error_page("Error copying directory '%s' (%s)\n", source, fs_strerror(fs_get_error()));
				return false;
			}
		} else {
			char full_path_in[MAX_STRINGLEN], full_path_out[MAX_STRINGLEN];;
			snprintf(full_path_in, MAX_STRINGLEN - 1, "%s/%s", source, fsl->filename);
			snprintf(full_path_out, MAX_STRINGLEN - 1, "%s/%s", destination, fsl->filename);
			if (!copy_file(full_path_in, full_path_out)) {
				return false;
			}
		}
		fsl = fsl->next;
	}
	return true;
}

void copy_userland(const char* devname) {
	/* Mount the newly formatted volume */
	progress = 0;
	block_progress = 0;
	display_progress("Copying system files (step 3 of 3)", 0);
	filesystem_mount("/harddisk", devname, "rfs");
	copy_directory("/system", "/harddisk/system");
	progress = 33;
	block_progress = 0;
	display_progress("Copying system files (step 3 of 3)", 33);
	copy_directory("/programs", "/harddisk/programs");
	progress = 66;
	block_progress = 0;
	display_progress("Copying system files (step 3 of 3)", 66);
	copy_directory("/images", "/harddisk/images");
	progress = 75;
	block_progress = 0;
	display_progress("Copying system files (step 3 of 3)", 75);
	fs_create_directory("/harddisk/boot");
	fs_create_directory("/harddisk/devices");
	progress = 100;
	block_progress = 0;
	display_progress("Copying system files (step 3 of 3)", 100);
}