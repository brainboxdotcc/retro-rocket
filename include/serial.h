/**
 * @file serial.h
 * @brief Simple serial output routines for Retro Rocket (QEMU COM1/COM2).
 *
 * Designed for write-only logging (e.g. profiler dumps).
 * No buffering is needed under QEMU, as it handles UART speed internally.
 */
#pragma once

#include "kernel.h"
#include <stdarg.h>
#include <stdint.h>

/* Standard COM port base addresses */
#define COM1 0x3F8
#define COM2 0x2F8

/**
 * @brief Initialise the serial port.
 *
 * @param port Base I/O port of the UART (e.g. COM1 or COM2).
 */
__attribute__((no_instrument_function)) void serial_init(uint16_t port);

/**
 * @brief Write a single character to the serial port.
 *
 * Blocks until transmitter is ready.
 *
 * @param port Base I/O port.
 * @param c Character to write.
 */
__attribute__((no_instrument_function)) void serial_putc(uint16_t port, char c);

/**
 * @brief Write a null-terminated string to the serial port.
 *
 * @param port Base I/O port.
 * @param s String to write.
 */
__attribute__((no_instrument_function)) void serial_write(uint16_t port, const char *s);

/**
 * @brief Formatted printf-style output to the serial port.
 *
 * @param port Base I/O port.
 * @param fmt Format string.
 * @param ... Arguments.
 */
__attribute__((no_instrument_function)) void serial_printf(uint16_t port, const char *fmt, ...) PRINTF_LIKE(2, 3);
