#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

// Mappings of IRQ numbers to interrupt numbers
#define IRQ0 32         // System timer
#define IRQ1 33         // Keyboard
#define IRQ2 34         // Cascade
#define IRQ3 35         // COM2
#define IRQ4 36         // COM1
#define IRQ5 37         // Sound
#define IRQ6 38         // FDD
#define IRQ7 39         // Parallel
#define IRQ8 40         // RTC
#define IRQ9 41         // Network, SCSI, unallocated
#define IRQ10 42        // Network, SCSI, unallocated
#define IRQ11 43        // Network, SCSI, unallocated
#define IRQ12 44        // PS2 Mouse
#define IRQ13 45        // FPU
#define IRQ14 46        // Primary IDE
#define IRQ15 47        // Secondary IDE

void Interrupt(u64 isrnumber, u64 errorcode);
void IRQ(u64 isrnumber, u64 errorcode);

// Interrupt handler definition
typedef void (*isr_t)(u8 isrnumber, u64 errorcode, u64 irqnumber);

void init_interrupts();

// Register a new interrupt handler
void register_interrupt_handler(u8 n, isr_t handler);

#endif
