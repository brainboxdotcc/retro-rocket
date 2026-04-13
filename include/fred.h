/**
 * @file fred.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 * FRED: Flexible Return and Event Delivery
 */
#pragma once
#include <stdint.h>

/**
 * @brief FRED stack pointer MSRs
 *
 * Each MSR defines the stack pointer used for a given FRED stack level.
 * The CPU may select one of these depending on event type and configuration.
 * All levels should be initialised to valid, aligned stacks.
 */
#define IA32_FRED_RSP0		0x1CC
#define IA32_FRED_RSP1		0x1CD
#define IA32_FRED_RSP2		0x1CE
#define IA32_FRED_RSP3		0x1CF

/**
 * @brief CPUID FRED feature bit
 */
#define CPUID_FEAT_FRED		17

/**
 * @brief FRED stack level configuration MSR
 *
 * Controls how many stack levels are active and how the CPU selects
 * between IA32_FRED_RSP0–RSP3 during event delivery.
 */
#define IA32_FRED_STKLVLS	0x1D0

/**
 * @brief FRED selector configuration MSR
 *
 * Defines code segment selectors used during privilege transitions.
 * Lower and upper halves correspond to kernel and user selectors.
 * For ring-0-only systems this may be unused but must remain valid.
 */
#define IA32_FRED_STAR		0x1D2

/**
 * @brief FRED entry configuration MSR
 *
 * Holds the entry point address and configuration for FRED event delivery.
 */
#define IA32_FRED_CONFIG	0x1D4

/**
 * @brief CR4.FRED enable bit
 *
 * Enables Flexible Return and Event Delivery when set.
 */
#define CR4_FRED		(1ULL << 32)

/**
 * @brief Size of each FRED stack
 *
 * Each stack should be 64 byte aligned
 */
#define USTACK_SIZE		4096

/**
 * @brief FRED event frame
 *
 * Saved processor state on entry via FRED. This replaces the traditional
 * IDT-based interrupt frame and includes general-purpose registers along
 * with architectural state
 *
 * Layout must match the hardware-defined frame exactly
 */
typedef struct fred_frame {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t error;      /**< Error code or event-specific value */
	uint64_t ip;         /**< Instruction pointer at time of event */
	uint64_t cs;         /**< Code segment */
	uint64_t flags;      /**< RFLAGS */
	uint64_t rsp;        /**< Stack pointer */
	uint64_t ss;         /**< Stack segment */
	uint64_t fred_event_data; /**< Additional event-specific data */
	uint64_t fred_reserved;   /**< Reserved, must be preserved */
} fred_frame_t;

/**
 * @brief Check if the current CPU supports FRED
 *
 * Uses CPUID to determine availability of the feature
 *
 * @return true if FRED is supported
 */
bool fred_supported(void);

/**
 * @brief Enable FRED on the current CPU
 *
 * Must be called per-CPU
 *
 * @return true on success
 */
bool enable_fred_for_this_cpu(void);

/**
 * @brief Initialise FRED subsystem
 *
 * Sets up stacks, MSRs, and entry points required for FRED operation.
 *
 * @return true on success
 */
bool init_fred(void);

/**
 * @brief Ring 0 FRED entry handler
 *
 * Called on event delivery after low-level entry stub.
 * Receives a pointer to the hardware-constructed FRED frame.
 *
 * @param frame Pointer to saved processor state
 */
void fred_ring0_entry(fred_frame_t* frame);