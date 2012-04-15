#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef u32 spinlock;

/* Function bodies for *_spinlock defined in asm/spinlock.asm */

void init_spinlock(spinlock* s);
void lock_spinlock(spinlock* s);
void unlock_spinlock(spinlock* s);

#endif

