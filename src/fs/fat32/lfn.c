#include "kernel.h"

/**
 * @brief Reads a chunk of a long filename from a directory entry.
 *
 * WHY THE HELL DID MICROSOFT MAKE THIS IN SUCH A WEIRD WAY?!
 * Who in their right mind thought it sensible to split up a name
 * across multiple different tiny arrays, and why is it 16 bit unicode
 * instead of UTF-8 and WHY is 0xffff considered an invalid character?!
 * Does NOT compute!!!
 *
 * @param nameptr pointer to name we are appending to
 * @param wide_chars wide characters to append
 * @param n_wide_chars number of wide characters to append
 * @return char* new name pointer
 */
char* parse_shitty_lfn_entry(char* nameptr, uint16_t* wide_chars, uint8_t n_wide_chars)
{
	for (int x = 0; x < n_wide_chars; ++x) {
		if (wide_chars[x] && wide_chars[x] != 0xffff) {
			if (wide_chars[x] < 0x80) {
				*nameptr++ = (char)wide_chars[x];
			} else {
				*nameptr++ = '?';
			}
		}
	}
	return nameptr;
}

uint8_t lfn_checksum(unsigned char* short_name)
{
	uint8_t sum = 0;
	for (uint16_t short_name_len = 11; short_name_len; --short_name_len) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *short_name++;
	}
	return sum;
}

void make_short(directory_entry_t* entry, const char* long_name, fs_directory_entry_t* current)
{
	memset(entry->name, ' ', 11);
	size_t end_number = 1;
	size_t x = 0;
	size_t n = 0;
	for (n = 0; n < 6; ++n) {
		while (!isalnum(long_name[x])) x++;
		if (long_name[x]) {
			entry->name[n] = toupper(long_name[x++]);
		}
	}
	entry->name[n++] = '~';
	/* Temporarily null terminate */
	entry->name[n + 2] = 0;
	bool found = false;
	do {
		found = false;
		entry->name[n] = (end_number / 10) + '0';
		entry->name[n + 1] = (end_number % 10) + '0';
		for (fs_directory_entry_t* e = current; e; e = e->next) {
			if (!memcmp(e->alt_filename, entry->name, 11)) {
				end_number++;
				found = true;
			}
		}
		if (end_number > 99) {
			return;
		}
	} while (found);
	entry->name[n + 2] = ' ';
}

#define FILL_UCS2(remain, number_chars, offset, member) \
	if (remain > offset) { \
		for (size_t n = 0; n < number_chars; ++n) { \
			lfn[*entry_count].member[n] = 0; \
			if (n + offset < remaining) { \
				lfn[*entry_count].member[n] = filename[n + offset]; \
			} \
		} \
	}

void build_lfn_chain(const char* filename, fs_directory_entry_t* current, directory_entry_t* short_entry, directory_entry_t** entries, size_t* entry_count)
{
	if (entry_count == NULL || entries == NULL || filename == NULL || current == NULL || short_entry == NULL) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return;
	}
	*entry_count = 0;
	size_t increment = 1;
	make_short(short_entry, filename, current);
	*entries = kmalloc(sizeof(lfn_t) * 65);
	if (!*entries) {
		fs_set_error(FS_ERR_OUT_OF_MEMORY);
		return;
	}
	lfn_t* lfn = (lfn_t*)*entries;
	memset(lfn, 0, sizeof(lfn_t) * 65);
	uint8_t checksum = lfn_checksum((unsigned char*)short_entry->name);
	for (; *filename;) {
		lfn[*entry_count].attributes = ATTR_LONG_NAME;
		lfn[*entry_count].order = increment++;
		lfn[*entry_count].entry_type = 0;
		lfn[*entry_count].reserved = 0;
		lfn[*entry_count].checksum = checksum;
		size_t remaining = strlen(filename);
		FILL_UCS2(remaining, 5, 0, first); /* Entry 0-4 (5 chars) */
		FILL_UCS2(remaining, 6, 5, second); /* entry 5-10 (6 chars) */
		FILL_UCS2(remaining, 2, 11, third); /* Entry 11-12 (2 chars)*/
		filename += (remaining > 13 ? 13 : remaining);
		lfn[*entry_count].order &= ~ATTR_LFN_LAST_ENTRY;
		if (!*filename) {
			lfn[*entry_count].order |= ATTR_LFN_LAST_ENTRY;
		}
		(*entry_count)++;
		if (*entry_count == 64) {
			return;
		}
	}
}

void parse_short_name(directory_entry_t* entry, char* name, char* dotless)
{
	if (!entry || !name || !dotless) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return;
	}
	strlcpy(name, entry->name, 9);
	char* trans;
	for (trans = name; *trans; ++trans) {
		if (*trans == ' ') {
			*trans = 0;
		}
	}
	strlcat(name, ".", 10);
	strlcat(name, &(entry->name[8]), 13);
	for (trans = name; *trans; ++trans) {
		if (*trans == ' ') {
			*trans = 0;
		}
	}

	size_t namelen = strlen(name);
	// remove trailing dot on dir names
	if (name[namelen - 1] == '.') {
		name[namelen - 1] = 0;
	}

	strlcpy(dotless, entry->name, 12);
	for (trans = dotless + 11; trans >= dotless; --trans) {
		if (*trans == ' ') {
			*trans = 0;
		}
	}
	dotless[12] = 0;
	name[12] = 0;
}

