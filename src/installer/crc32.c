#include <installer.h>

/* ---------------- CRC32 (poly 0xEDB88320) ---------------- */

uint32_t crc32_update(uint32_t crc, const void *data, size_t len)
{
	static uint32_t table[256];
	static bool init = false;

	if (!init) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int j = 0; j < 8; j++) {
				if ((c & 1U) != 0U) {
					c = 0xEDB88320U ^ (c >> 1);
				} else {
					c >>= 1;
				}
			}
			table[i] = c;
		}
		init = true;
	}

	uint32_t c = crc ^ 0xFFFFFFFFU;
	const uint8_t *p = (const uint8_t *)data;

	for (size_t i = 0; i < len; i++) {
		c = table[(c ^ p[i]) & 0xFFU] ^ (c >> 8);
	}

	return c ^ 0xFFFFFFFFU;
}