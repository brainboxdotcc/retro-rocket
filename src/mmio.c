/**
 * @file mmio.h
 * @brief Provides helper functions for memory mapped IO reads/writes
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#include <mmio.h>

uint8_t mmio_read8(uint64_t p_address) {
	return *((volatile uint8_t *) (p_address));
}

uint16_t mmio_read16(uint64_t p_address) {
	return *((volatile uint16_t *) (p_address));
}

uint32_t mmio_read32(uint64_t p_address) {
	return *((volatile uint32_t *) (p_address));
}

uint64_t mmio_read64(uint64_t p_address) {
	return *((volatile uint64_t *) (p_address));
}

void mmio_write8(uint64_t p_address, uint8_t p_value) {
	(*((volatile uint8_t *) (p_address))) = (p_value);
}

void mmio_write16(uint64_t p_address, uint16_t p_value) {
	(*((volatile uint16_t *) (p_address))) = (p_value);
}

void mmio_write32(uint64_t p_address, uint32_t p_value) {
	(*((volatile uint32_t *) (p_address))) = (p_value);

}

void mmio_write64(uint64_t p_address, uint64_t p_value) {
	(*((volatile uint64_t *) (p_address))) = (p_value);
}

