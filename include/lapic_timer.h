/**
 * @file lapic_timer.h
 * @brief Definitions and helpers for the Local APIC timer.
 *
 * Provides register offsets and configuration flags for the
 * Local Advanced Programmable Interrupt Controller (LAPIC) timer,
 * as well as an initialisation routine for setting up timer ticks.
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

#define APIC_SVR         0x0F0  /**< Spurious Interrupt Vector Register */
#define APIC_TASKPRIOR   0x080  /**< Task Priority Register */
#define APIC_EOI         0x0B0  /**< End of Interrupt Register */
#define APIC_LDR         0x0D0  /**< Logical Destination Register */
#define APIC_DFR         0x0E0  /**< Destination Format Register */
#define APIC_SPURIOUS    0x0F0  /**< Spurious Interrupt Vector Register */
#define APIC_ESR         0x280  /**< Error Status Register */
#define APIC_ICRL        0x300  /**< Interrupt Command Register (low) */
#define APIC_ICRH        0x310  /**< Interrupt Command Register (high) */
#define APIC_LVT_TMR     0x320  /**< Local Vector Table (Timer) */
#define APIC_LVT_PERF    0x340  /**< Local Vector Table (Performance Counter) */
#define APIC_LVT_LINT0   0x350  /**< Local Vector Table (LINT0) */
#define APIC_LVT_LINT1   0x360  /**< Local Vector Table (LINT1) */
#define APIC_LVT_ERR     0x370  /**< Local Vector Table (Error) */
#define APIC_TMRINITCNT  0x380  /**< Initial Count Register (Timer) */
#define APIC_TMRCURRCNT  0x390  /**< Current Count Register (Timer) */
#define APIC_TMRDIV      0x3E0  /**< Divide Configuration Register */
#define APIC_LAST        0x38F  /**< Last valid APIC register index */

#define APIC_DISABLE     0x10000   /**< Disable a given APIC function */
#define APIC_SW_ENABLE   0x00100   /**< Enable APIC in software */
#define APIC_CPUFOCUS    0x00200   /**< CPU focus (for broadcast IPIs) */
#define APIC_NMI         (4 << 8)  /**< Non-Maskable Interrupt vector */
#define TMR_PERIODIC     0x20000   /**< Timer operates in periodic mode */
#define TMR_BASEDIV      (1 << 20) /**< Base divider for timer frequency */

/**
 * @brief Initialise and calibrate the Local APIC timer.
 *
 * This function calibrates the LAPIC timer against a stable external timebase
 * and configures it to generate periodic interrupts at a frequency derived
 * from the specified quantum.
 *
 * Calibration is performed using the ACPI Power Management (PM) timer if it
 * is available. The PM timer provides a fixed-frequency reference at
 * 3.579545 MHz, and calibration is performed over ~100 ms for accuracy. If
 * the PM timer is not available, the function falls back to using the RTC
 * seconds counter, which requires a 1-second sample period.
 *
 * Once calibrated, the LAPIC timer is programmed in periodic mode with a
 * divisor of 16, and interrupts are routed to IRQ16.
 *
 * @param quantum Desired time slice quantum, in ticks per second.
 * Must be greater than zero.
 */
void init_lapic_timer(uint64_t quantum);
