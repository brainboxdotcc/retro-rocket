#include <kernel.h>

static const uint8_t hex[256] = {
	['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3,
	['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7,
	['8'] = 8, ['9'] = 9,
	['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
	['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
};

static const char hex_bin[] = "0123456789ABCDEF";

static inline __attribute__((always_inline)) uint8_t hexval(char c)
{
	return hex[(uint8_t)c];
}

static inline __attribute__((always_inline)) uint8_t parse_byte(const char* s)
{
	return (hexval(s[0]) << 4) | hexval(s[1]);
}

static inline void guid_write_byte(char** out, uint8_t value) {
	*(*out)++ = hex_bin[value >> 4];
	*(*out)++ = hex_bin[value & 0x0f];
}

bool guid_to_binary(const text_guid_t guid, binary_guid_t binary) {
	static const uint8_t order[] = { 3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15 };
	static const uint8_t offset[] = { 0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };
	for (size_t i = 0; i < sizeof(order); i++) {
		binary[order[i]] = parse_byte(guid + offset[i]);
	}
	return true;
}

bool binary_to_guid(const binary_guid_t binary, text_guid_t guid) {
	static const uint8_t order[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-', 8, 9, '-', 10, 11, 12, 13, 14, 15 };
	const uint8_t* in = binary;
	char* out = guid;
	for (size_t i = 0; i < sizeof(order); i++) {
		if (order[i] == '-') {
			*out++ = '-';
			continue;
		}
		guid_write_byte(&out, in[order[i]]);
	}
	*out = 0;
	return true;
}

bool match_guid_binary(const binary_guid_t a, const binary_guid_t b) {
	return !memcmp(a, b, sizeof(binary_guid_t));
}

bool match_guid_text(const text_guid_t a, const text_guid_t b) {
	return !stricmp(a, b);
}