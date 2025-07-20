/**
 * @file ioapic.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

void ioapic_redir_set(uint32_t irq, uint32_t vector, uint32_t del_mode, uint32_t dest_mode, uint32_t intpol, uint32_t trigger_mode, uint32_t mask);
void ioapic_redir_get(uint32_t gsi, uint32_t* vector, uint32_t* del_mode, uint32_t* dest_mode, uint32_t* intpol, uint32_t* trigger_mode, uint32_t* mask, uint32_t* destination);
void ioapic_redir_unmask(uint32_t gsi);
void ioapic_redir_set_precalculated(uint32_t gsi, uint32_t upper, uint32_t lower);
