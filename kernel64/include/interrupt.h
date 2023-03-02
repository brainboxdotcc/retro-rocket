#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

void Interrupt(u64 isrnumber, u64 errorcode);
void IRQ(u64 isrnumber, u64 errorcode);

#endif
