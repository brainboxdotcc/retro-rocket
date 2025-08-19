#include <kernel.h>

void ata_copy_str(char *dst, size_t dst_len, const uint8_t *id_page, int word_off, int word_cnt) {
	size_t n = word_cnt * 2;
	if (n >= dst_len) {
		n = dst_len - 1;
	}

	for (size_t i = 0; i < n; i += 2) {
		dst[i] = (char)id_page[(word_off * 2) + i + 1];
		dst[i + 1] = (char)id_page[(word_off * 2) + i + 0];
	}
	dst[n] = '\0';

	for (long j = n - 1; j >= 0; j--) {
		if (dst[j] == ' ') {
			dst[j] = '\0';
		} else {
			break;
		}
	}
}

void humanise_capacity(char *out, size_t out_len, uint64_t bytes) {
	const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
	int u = 0;
	uint64_t v = bytes;

	while (v >= 1024 && u < 5) {
		v /= 1024;
		u++;
	}

	snprintf(out, out_len, "%lu %s", v, units[u]);
}

void build_sata_label(storage_device_t *sd, const uint8_t *id_page) {
	char model[48] = {0};
	char size_str[24] = {0};
	uint16_t w217 = ((const uint16_t *)id_page)[217]; /* 1 = SSD */
	const char *kind = (w217 == 1) ? "SSD" : "HDD";

	ata_copy_str(model, sizeof(model), id_page, 27, 20);
	humanise_capacity(size_str, sizeof(size_str), sd->size * sd->block_size);
	snprintf(sd->ui.label, sizeof(sd->ui.label), "%s - %s (%s)", (model[0] ? model : "ATA Device"), size_str, kind);
	sd->ui.is_optical = false;
}
