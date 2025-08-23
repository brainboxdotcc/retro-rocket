#include <kernel.h>

int storage_device_ahci_block_read(void* dev, uint64_t start, uint32_t bytes, unsigned char* buffer) {
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd || !buffer || bytes == 0) {
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	unsigned char* end = buffer + bytes;

	ahci_hba_mem_t* abar = sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];

	const uint32_t max_per_cmd = 16;

	if (check_type(port) == AHCI_DEV_SATAPI) {
		while (sectors > 0) {
			uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
			if (!ahci_atapi_read(port, start, this_xfer, (char*)buffer, abar)) {
				return false;
			}
			start += this_xfer;
			buffer += this_xfer * sd->block_size;
			sectors -= this_xfer;
		}
		return true;
	} else {
		while (sectors > 0) {
			uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
			uint32_t bytes_this = this_xfer * sd->block_size;

			if (buffer + bytes_this > end) {
				uint64_t bytes_left = (uint64_t)(end - buffer);
				this_xfer = bytes_left / sd->block_size;
				if (this_xfer == 0) {
					break;
				}
				bytes_this = this_xfer * sd->block_size;
			}

			if (!ahci_read(port, start, this_xfer, (char*)buffer, abar)) {
				return 0;
			}

			start += this_xfer;
			buffer += bytes_this;
			sectors -= this_xfer;
		}
		return 1;
	}
}

bool storage_device_ahci_block_clear(void *dev, uint64_t lba, uint32_t bytes) {
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	ahci_hba_mem_t* abar = sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];
	ahci_trim_caps* caps = sd->opaque3;
	if (caps && caps->has_trim) {
		return ahci_trim_one_range(port, abar, caps, lba, sectors) > 0;
	}
	return false;
}

int storage_device_ahci_block_write(void* dev, uint64_t start, uint32_t bytes, const unsigned char* buffer) {
	storage_device_t* sd = (storage_device_t*)dev;
	if (!sd) {
		return 0;
	}

	uint32_t sectors = (bytes + sd->block_size - 1) / sd->block_size;
	if (sectors < 1) {
		sectors = 1;
	}

	ahci_hba_mem_t* abar = sd->opaque2;
	ahci_hba_port_t* port = &abar->ports[sd->opaque1];

	const uint32_t max_per_cmd = 16;
	while (sectors > 0) {
		uint32_t this_xfer = (sectors > max_per_cmd) ? max_per_cmd : sectors;
		if (!ahci_write(port, start, this_xfer, (char*)buffer, abar)) {
			return false;
		}
		start   += this_xfer;
		buffer  += this_xfer * sd->block_size;
		sectors -= this_xfer;
	}

	return true;
}

