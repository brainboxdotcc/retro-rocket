/**
 * @file mmio.h
 * @brief Helper functions for memory‑mapped I/O (MMIO) access.
 *
 * Provides a consistent interface for reading and writing 8‑, 16‑, 32‑,
 * and 64‑bit values from/to memory‑mapped I/O regions. All addresses
 * are passed as 64‑bit physical addresses.
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

/* ------------------------------------------------------------------------- */
/* MMIO Read Functions                                                        */
/* ------------------------------------------------------------------------- */

/**
 * @brief Read an 8‑bit value from a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @return The 8‑bit value read.
 */
uint8_t mmio_read8(uint64_t p_address);

/**
 * @brief Read a 16‑bit value from a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @return The 16‑bit value read.
 */
uint16_t mmio_read16(uint64_t p_address);

/**
 * @brief Read a 32‑bit value from a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @return The 32‑bit value read.
 */
uint32_t mmio_read32(uint64_t p_address);

/**
 * @brief Read a 64‑bit value from a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @return The 64‑bit value read.
 */
uint64_t mmio_read64(uint64_t p_address);

/* ------------------------------------------------------------------------- */
/* MMIO Write Functions                                                       */
/* ------------------------------------------------------------------------- */

/**
 * @brief Write an 8‑bit value to a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @param p_value Value to write.
 */
void mmio_write8(uint64_t p_address, uint8_t p_value);

/**
 * @brief Write a 16‑bit value to a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @param p_value Value to write.
 */
void mmio_write16(uint64_t p_address, uint16_t p_value);

/**
 * @brief Write a 32‑bit value to a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @param p_value Value to write.
 */
void mmio_write32(uint64_t p_address, uint32_t p_value);

/**
 * @brief Write a 64‑bit value to a memory‑mapped I/O register.
 * @param p_address Physical address of the register.
 * @param p_value Value to write.
 */
void mmio_write64(uint64_t p_address, uint64_t p_value);
