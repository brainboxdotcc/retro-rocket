#ifndef TIMER_H
#define TIMER_H

void sleep_one_tick();
void init_timer(u32 frequency);
void beep(u32 pitch);
void stopbeep();
u64 get_ticks();

#endif

