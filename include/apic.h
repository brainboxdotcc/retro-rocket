/**
 * @file apic.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#ifndef __APIC_H__
#define __APIC_H__

#define APIC_ADDRESS 0x4000
#define APIC_BASE_MSR 0x1B
#define APIC_BASE_MSR_ENABLE 0x800

// All of these values are offset from the APIC base address
#define APIC_ID 0x0020
#define APIC_VERSION 0x0030

uint32_t apic_read(uint64_t reg);
void apic_write(uint64_t reg, uint32_t value);
uint8_t cpu_id();
uint64_t get_lapic_address();

#endif
