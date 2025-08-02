/**
 * @file ioapic.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief Provides functions for manipulating the I/O APIC redirection table.
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

/**
 * @brief Configure a redirection entry in the I/O APIC.
 *
 * @param irq           The IRQ line to configure (relative to the IOAPIC).
 * @param vector        The interrupt vector to deliver to the CPU.
 * @param del_mode      Delivery mode (e.g. fixed, lowest priority, NMI).
 * @param dest_mode     Destination mode (physical or logical).
 * @param intpol        Interrupt polarity (active high or low).
 * @param trigger_mode  Trigger mode (edge or level triggered).
 * @param mask          Whether to mask (disable) this interrupt line.
 */
void ioapic_redir_set(uint32_t irq, uint32_t vector, uint32_t del_mode, uint32_t dest_mode, uint32_t intpol, uint32_t trigger_mode, uint32_t mask);

/**
 * @brief Read a redirection entry from the I/O APIC.
 *
 * @param gsi           Global System Interrupt number to query.
 * @param vector        Out: interrupt vector.
 * @param del_mode      Out: delivery mode.
 * @param dest_mode     Out: destination mode.
 * @param intpol        Out: polarity.
 * @param trigger_mode  Out: trigger mode.
 * @param mask          Out: mask state.
 * @param destination   Out: APIC ID of the destination CPU.
 */
void ioapic_redir_get(uint32_t gsi, uint32_t* vector, uint32_t* del_mode, uint32_t* dest_mode, uint32_t* intpol, uint32_t* trigger_mode, uint32_t* mask, uint32_t* destination);

/**
 * @brief Unmask (enable) a redirection entry for a given GSI.
 *
 * @param gsi The Global System Interrupt number to unmask.
 */
void ioapic_redir_unmask(uint32_t gsi);

/**
 * @brief Write a redirection entry using pre‑calculated 64‑bit values.
 *
 * @param gsi   Global System Interrupt number to set.
 * @param upper Upper 32 bits of the redirection entry.
 * @param lower Lower 32 bits of the redirection entry.
 */
void ioapic_redir_set_precalculated(uint32_t gsi, uint32_t upper, uint32_t lower);

/**
 * @brief Set or clear the mask bit for a redirection entry.
 *
 * @param gsi     Global System Interrupt number to update.
 * @param masked  True to mask (disable), false to unmask (enable).
 */
void ioapic_mask_set(uint32_t gsi, bool masked);
