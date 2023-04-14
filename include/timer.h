#pragma once

#include <stdint.h>

static inline uint64_t rdtsc()
{
    uint64_t ret;
    __asm__ volatile ("rdtsc":"=A"(ret));
    return ret;
}

void sleep_one_tick();
void beep(uint32_t pitch);
void stopbeep();
void sleep(uint64_t secs);
uint64_t get_ticks();
void timer_callback(uint8_t isr, uint64_t errorcode, uint64_t irq, void* opaque);
