#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Writes the CPUID information into the provided context for a specific leaf.
 *
 * This function retrieves the CPUID information for a given leaf and stores the results
 * in the `last_cpuid_result` field of the provided `basic_ctx` context.
 *
 * @param ctx The BASIC context where the CPUID result is stored.
 * @param leaf The CPUID leaf to query.
 */
void write_cpuid(struct basic_ctx* ctx, int leaf);

/**
 * @brief Writes the CPUID information into the provided context for a specific leaf and subleaf.
 *
 * This function retrieves the CPUID information for a given leaf and subleaf and stores the
 * results in the `last_cpuid_result` field of the provided `basic_ctx` context.
 *
 * @param ctx The BASIC context where the CPUID result is stored.
 * @param leaf The CPUID leaf to query.
 * @param subleaf The CPUID subleaf to query.
 */
void write_cpuidex(struct basic_ctx* ctx, int leaf, int subleaf);

/**
 * @brief Retrieves a specific CPUID register value from the context.
 *
 * This function retrieves the value of a specific CPUID register (eax, ebx, ecx, edx)
 * from the `last_cpuid_result` in the provided `basic_ctx`.
 *
 * @param ctx The BASIC context where the CPUID result is stored.
 * @param reg The register to retrieve (0 = eax, 1 = ebx, 2 = ecx, 3 = edx).
 * @return The value of the specified CPUID register.
 */
int64_t get_cpuid_reg(struct basic_ctx* ctx, int64_t reg);

/**
 * @brief Executes a legacy CPUID instruction for a specific leaf and subleaf.
 *
 * This function executes the CPUID instruction with a specified leaf and subleaf and
 * stores the result in the context's `last_cpuid_result`.
 *
 * @param ctx The BASIC context where the CPUID result is stored.
 * @return 1 if the subleaf is not -1, 0 if it is.
 */
int64_t basic_legacy_cpuid(struct basic_ctx* ctx);

/**
 * @brief Retrieves the last CPUID register value from the context.
 *
 * This function retrieves the most recent CPUID register value stored in the
 * `last_cpuid_result` from the provided `basic_ctx`.
 *
 * @param ctx The BASIC context where the CPUID result is stored.
 * @return The value of the last CPUID register queried.
 */
int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx);

/**
 * @brief Retrieves the CPU brand string.
 *
 * This function retrieves the CPU brand string by executing CPUID with leaf values
 * 0x80000002, 0x80000003, and 0x80000004 and concatenating the result.
 *
 * @param ctx The BASIC context.
 * @return The CPU brand string.
 */
char* basic_cpugetbrand(struct basic_ctx* ctx);

/**
 * @brief Retrieves the CPU vendor string.
 *
 * This function retrieves the CPU vendor string by executing CPUID with leaf value 0
 * and storing the result in the `last_cpuid_result`.
 *
 * @param ctx The BASIC context.
 * @return The CPU vendor string.
 */
char* basic_cpugetvendor(struct basic_ctx* ctx);

/**
 * @brief Converts an integer value to a string representation.
 *
 * This function converts the specified integer value into its ASCII representation
 * as a string, with a specified length.
 *
 * @param ctx The BASIC context.
 * @return The string representation of the integer value.
 */
char* basic_intoasc(struct basic_ctx* ctx);

/**
 * @brief Retrieves the CPUID result for a specific leaf and subleaf.
 *
 * This function executes CPUID for a specified leaf and subleaf and returns the
 * value of a specific CPUID register as defined by the `reg` parameter.
 *
 * @param ctx The BASIC context.
 * @return The CPUID result of the specified register.
 */
int64_t basic_cpuid(struct basic_ctx* ctx);
