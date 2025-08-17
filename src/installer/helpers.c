#include <installer.h>

uint64_t align_up_u64(uint64_t x, uint64_t a)
{
	if (a == 0ULL) {
		return x;
	}
	return (x + (a - 1ULL)) & ~(a - 1ULL);
}

void make_utf16le_name(const char *ascii, uint16_t out36[36])
{
	size_t n = strlen(ascii);
	if (n > 36U) {
		n = 36U;
	}
	for (size_t i = 0; i < n; i++) {
		out36[i] = (uint16_t)(uint8_t)ascii[i];
	}
	for (size_t i = n; i < 36U; i++) {
		out36[i] = 0;
	}
}

void random_guid_v4(uint8_t out16[16])
{
	for (int i = 0; i < 16; i += 4) {
		uint32_t r = rand();
		memcpy(out16 + i, &r, 4);
	}
	out16[6] = (out16[6] & 0x0F) | 0x40; /* version 4 */
	out16[8] = (out16[8] & 0x3F) | 0x80; /* variant 10 */
}

bool write_lbas(const char *devname, uint64_t start_lba, const void *buf, uint32_t bytes, uint32_t sector_bytes)
{
	if ((bytes % sector_bytes) != 0) {
		return false;
	}
	if (write_storage_device(devname, start_lba, bytes, (const unsigned char *)buf) == 0) {
		return false;
	}
	return true;
}
