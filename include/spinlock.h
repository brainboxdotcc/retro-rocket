/**
 * @file spinlock.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2023
 */
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef uint32_t spinlock;

/* Function bodies for *_spinlock defined in asm/spinlock.asm */

void init_spinlock(spinlock* s);
void lock_spinlock(spinlock* s);
void unlock_spinlock(spinlock* s);

void get_ioapic_address(uint64_t* apic);

#endif

