/**
 * @file timer.h
 * @brief Timer and delay functions (APIC-driven, 100 Hz tick rate).
 *
 * Timers are based on the Local APIC timer, running at a fixed
 * resolution of 1/100th of a second (10 ms). A global tick counter
 * is incremented on each timer interrupt, and can be queried via
 * get_ticks(). Sleep functions busy-wait or HLT until the tick
 * counter advances.
 *
 * The PIT (8253/8254) is also configured to support PC speaker
 * tone generation for beep().
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <stdint.h>

/**
 * @brief Read the current Time Stamp Counter.
 *
 * Uses the RDTSC instruction to obtain the raw CPU cycle counter.
 * This value is CPU-local and not synchronised across cores.
 *
 * @return Current TSC value.
 */
static inline uint64_t rdtsc(void) {
	uint32_t lo, hi;
	__asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

/**
 * @brief Sleep until the next timer tick.
 *
 * Returns once the global tick counter advances by at least one.
 * This is approximately a 10 ms delay at 100 Hz.
 */
void sleep_one_tick(void);

/**
 * @brief Generate a short PC speaker beep at the given pitch.
 *
 * The tone automatically stops after ~0.5 seconds, controlled by
 * the timer interrupt.
 *
 * @note This only works if PIT channel 2 is wired to a physical speaker,
 * which is not the case on many modern systems. Reliable only in QEMU/Bochs
 * and older PCs.
 *
 * @param pitch Frequency in Hz.
 */
void beep(uint32_t pitch);

/**
 * @brief Immediately stop any ongoing PC speaker beep.
 *
 * @note This only works if PIT channel 2 is wired to a physical speaker,
 * which is not the case on many modern systems. Reliable only in QEMU/Bochs
 * and older PCs.
 */
void stopbeep(void);

/**
 * @brief Sleep for the specified number of seconds.
 *
 * Uses the global tick counter to wait until the required number
 * of seconds has elapsed. CPU executes HLT while waiting.
 *
 * @param secs Number of seconds to sleep.
 */
void sleep(uint64_t secs);

/**
 * @brief Get the current global tick count.
 *
 * Incremented by the Local APIC timer interrupt at 100 Hz.
 *
 * @return Number of ticks since timer initialisation.
 */
uint64_t get_ticks(void);

/**
 * @brief Timer interrupt callback.
 *
 * Invoked by the Local APIC (or PIT fallback) at 100 Hz. Increments
 * the global tick counter, executes registered idle timer callbacks,
 * auto-flips the framebuffer if enabled, and handles PC speaker timeouts.
 *
 * This should not normally be called by user code.
 *
 * @param isr       Interrupt vector number.
 * @param errorcode Error code pushed by CPU (if any).
 * @param irq       IRQ number (APIC routing).
 * @param opaque    Opaque pointer for ISR framework use.
 */
void timer_callback(uint8_t isr, uint64_t errorcode, uint64_t irq, void* opaque);
