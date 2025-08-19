#include <kernel.h>

void trim_trailing_spaces(char *s) {
	size_t n = strlen(s);
	while (n > 0) {
		if (s[n - 1] == ' ') {
			s[n - 1] = '\0';
			n--;
		} else {
			break;
		}
	}
}

void build_atapi_label(storage_device_t *sd, const uint8_t *inq) {
	char vendor[16] = {0};
	char product[32] = {0};
	char rev[16] = {0};

	memcpy(vendor,  inq + 8,  8);
	memcpy(product, inq + 16, 16);
	memcpy(rev,     inq + 32, 4);

	trim_trailing_spaces(vendor);
	trim_trailing_spaces(product);
	trim_trailing_spaces(rev);

	if (rev[0] != '\0') {
		snprintf(sd->ui.label, sizeof(sd->ui.label), "%s %s - %s (Optical)", vendor, product, rev);
	} else {
		snprintf(sd->ui.label, sizeof(sd->ui.label), "%s %s (Optical)", vendor, product);
	}

	sd->ui.is_optical = true;
}

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
