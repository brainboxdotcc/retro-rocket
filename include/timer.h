#ifndef TIMER_H
#define TIMER_H

void sleep_one_tick();
void beep(uint32_t pitch);
void stopbeep();
void sleep(uint64_t secs);
uint64_t get_ticks();
void timer_callback(uint8_t isr, uint64_t errorcode, uint64_t irq);

#endif

