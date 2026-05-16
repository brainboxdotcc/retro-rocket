#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <buddy_allocator.h>

/**
 * @brief Memory access grant node used by restricted BASIC contexts
 *
 * Each node represents a contiguous granted memory region which may be
 * accessed by a BASIC interpreter context when memory restrictions are
 * enabled
 *
 * The tree is ordered by base address and implemented as a red/black tree
 * for predictable lookup performance
 */
typedef struct memory_grant {
	uint8_t *base;			/**< Start of granted memory region */
	uint8_t *end;			/**< Inclusive end of granted memory region */
	struct memory_grant *left;	/**< Left child */
	struct memory_grant *right;	/**< Right child */
	struct memory_grant *parent;	/**< Parent node */
	bool red;			/**< Red/black tree colour flag */
} memory_grant_t;

/**
 * @brief Root object for a memory grant tree.
 *
 * Each BASIC interpreter context typically owns one instance of this
 * structure which tracks all memory regions granted to that context
 */
typedef struct memory_grants {
	memory_grant_t *root;		/**< Root node of the red/black tree */
} memory_grants_t;

/**
 * @brief Initialise a memory grant tree
 *
 * @param grants Grant tree to initialise
 */
void memory_grants_init(memory_grants_t *grants);

/**
 * @brief Add a granted memory region
 *
 * Grants a contiguous address range to a BASIC interpreter context
 *
 * @param grants     Grant tree
 * @param allocator  Allocator used for internal tree node allocation
 * @param base       Base address of granted region
 * @param length     Length of granted region in bytes
 * @return true on success, false on failure
 *
 * @note Regions are keyed by base address and may not overlap existing
 *       entries with the same base pointer.
 */
bool memory_grants_add(memory_grants_t *grants, buddy_allocator_t *allocator, void *base, size_t length);

/**
 * @brief Remove a granted memory region
 *
 * Removes a previously granted region identified by its base address
 *
 * @param grants     Grant tree
 * @param allocator  Allocator used to free internal tree nodes
 * @param base       Base address originally passed to memory_grants_add()
 * @return true if the region existed and was removed, false otherwise
 */
bool memory_grants_remove(memory_grants_t *grants, buddy_allocator_t *allocator, void *base);

/**
 * @brief Check whether an address range is fully granted
 *
 * @param grants  Grant tree
 * @param addr    Address to test
 * @param length  Length of requested access in bytes
 * @return true if the entire range is permitted, false otherwise
 *
 * Used by restricted BASIC contexts to validate direct memory access
 * operations such as PEEK, POKE, BINREAD, BINWRITE, and related functions
 */
bool memory_grants_contains(memory_grants_t *grants, void *addr, size_t length);

/**
 * @brief Validate an allocation base address
 *
 * Checks whether the supplied address exactly matches the base address of
 * a granted allocation
 *
 * @param grants  Grant tree
 * @param base    Allocation base address to validate
 * @return true if the base address is valid, false otherwise
 *
 * This is typically used to validate MEMRELEASE operations and prevent
 * invalid frees, interior-pointer frees, and double frees
 */
bool memory_grants_valid_base(memory_grants_t *grants, void *base);