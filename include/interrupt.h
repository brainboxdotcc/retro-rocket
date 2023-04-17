/**
 * @file interrupt.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2023
 */
#pragma once

#include "kernel.h"

// Mappings of IRQ numbers to interrupt numbers
enum irq_number_t {
	IRQ0  = 32,	 // System timer
	IRQ1  = 33,	 // Keyboard
	IRQ2  = 34,	 // Cascade
	IRQ3  = 35,	 // COM2
	IRQ4  = 36,	 // COM1
	IRQ5  = 37,	 // Sound
	IRQ6  = 38,	 // FDD
	IRQ7  = 39,	 // Parallel
	IRQ8  = 40,	 // RTC
	IRQ9  = 41,	 // Network, SCSI, unallocated
	IRQ10 = 42,	// Network, SCSI, unallocated
	IRQ11 = 43,	// Network, SCSI, unallocated
	IRQ12 = 44,	// PS2 Mouse
	IRQ13 = 45,	// FPU
	IRQ14 = 46,	// Primary IDE
	IRQ15 = 47,	// Secondary IDE
	IRQ16 = 50,	// LAPIC timer vector
};

/**
 * @brief A definition for an interrupt handler
 */
typedef void (*isr_t)(uint8_t isrnumber, uint64_t errorcode, uint64_t irqnumber, void* opaque);

/**
 * @brief An entry in a linked list of handlers attached to an ISR
 */
typedef struct shared_interrupt_t {
	isr_t interrupt_handler;
	pci_dev_t device;
	void* opaque;
	struct shared_interrupt_t* next;
} shared_interrupt_t;

/**
 * @brief Register an interrupt handler
 * 
 * @param n Interrupt number, IRQs start at 32 as per irq_number_t
 * @param handler IRQ handler function
 * @param device For PCI devices, specify the device node in this parameter.
 * This is to allow for IRQ sharing.
 * @param opaque An opaque pointer that can be passed back to your interrupt handler
 */
void register_interrupt_handler(uint8_t n, isr_t handler, pci_dev_t device, void* opaque);

/**
 * @brief Global routing function for interrupts (ISR number < 32)
 * 
 * @param isrnumber ISR number
 * @param errorcode Error code
 */
void Interrupt(uint64_t isrnumber, uint64_t errorcode);

/**
 * @brief Global routing function for IRQs (ISR number >= 32)
 * 
 * @param isrnumber ISR number
 * @param errorcode Error code (always zero)
 */
void IRQ(uint64_t isrnumber, uint64_t errorcode);

