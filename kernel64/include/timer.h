#ifndef TIMER_H
#define TIMER_H

void sleep_one_tick();
void init_timer(uint32_t frequency);
void beep(uint32_t pitch);
void stopbeep();
void sleep(uint64_t secs);
void sleep_one_tick();
uint64_t get_ticks();

#endif

