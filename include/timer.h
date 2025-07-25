/**
 * @file timer.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <stdint.h>

static inline uint64_t rdtsc(void) {
	uint32_t lo, hi;
	__asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

void sleep_one_tick();
void beep(uint32_t pitch);
void stopbeep();
void sleep(uint64_t secs);
uint64_t get_ticks();
void timer_callback(uint8_t isr, uint64_t errorcode, uint64_t irq, void* opaque);
