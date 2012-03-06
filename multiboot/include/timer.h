#ifndef TIMER_H
#define TIMER_H

void sleep_one_tick();
void init_timer(u32int frequency);
void beep(u32int pitch);
void stopbeep();
u64int get_ticks();

#endif

