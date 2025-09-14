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

/**
 * @brief Implements the BASIC statement OUTPORT.
 *
 * BASIC syntax:
 * @code
 *   OUTPORT port, value
 * @endcode
 *
 * Writes an 8-bit value to the specified I/O port.
 *
 * @param ctx Interpreter context (supplies arguments and error reporting).
 *
 * @note The value is truncated to 8 bits before being written.
 * @warning Raises a runtime error if the number or type of arguments is invalid.
 */
void outport_statement(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC statement OUTPORTD.
 *
 * BASIC syntax:
 * @code
 *   OUTPORTD port, value
 * @endcode
 *
 * Writes a 32-bit value to the specified I/O port.
 *
 * @param ctx Interpreter context.
 *
 * @note The value is truncated to 32 bits before being written.
 * @warning Raises a runtime error if the number or type of arguments is invalid.
 */
void outportd_statement(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC statement OUTPORT.
 *
 * Writes an 8-bit value to an I/O port.
 *
 * BASIC syntax:
 * @code
 *   OUTPORT port, value
 * @endcode
 *
 * @param ctx Interpreter context (provides parameters and error reporting).
 *
 * @note The value is truncated to 8 bits before the write.
 * @warning Raises a runtime error if the argument count or types are invalid.
 */
void outport_statement(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC statement OUTPORTW.
 *
 * Writes a 16-bit value to an I/O port.
 *
 * BASIC syntax:
 * @code
 *   OUTPORTW port, value
 * @endcode
 *
 * @param ctx Interpreter context.
 *
 * @note The value is truncated to 16 bits before the write.
 * @warning Raises a runtime error if the argument count or types are invalid.
 */
void outportw_statement(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC statement OUTPORTD.
 *
 * Writes a 32-bit value to an I/O port.
 *
 * BASIC syntax:
 * @code
 *   OUTPORTD port, value
 * @endcode
 *
 * @param ctx Interpreter context.
 *
 * @note The value is truncated to 32 bits before the write.
 * @warning Raises a runtime error if the argument count or types are invalid.
 */
void outportd_statement(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC function INPORT.
 *
 * Reads an 8-bit value from an I/O port.
 *
 * BASIC syntax:
 * @code
 *   v = INPORT(port)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return 8-bit value (0–255) widened to int64_t.
 *
 * @warning Raises a runtime error if the argument count or types are invalid.
 */
int64_t basic_inport(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC function INPORTW.
 *
 * Reads a 16-bit value from an I/O port.
 *
 * BASIC syntax:
 * @code
 *   v = INPORTW(port)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return 16-bit value (0–65535) widened to int64_t.
 *
 * @warning Raises a runtime error if the argument count or types are invalid.
 */
int64_t basic_inportw(struct basic_ctx* ctx);

/**
 * @brief Implements the BASIC function INPORTD.
 *
 * Reads a 32-bit value from an I/O port.
 *
 * BASIC syntax:
 * @code
 *   v = INPORTD(port)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return 32-bit value widened to int64_t.
 *
 * @warning Raises a runtime error if the argument count or types are invalid.
 */
int64_t basic_inportd(struct basic_ctx* ctx);

/**
 * @brief Implements BITOR(a, b) — bitwise OR.
 *
 * BASIC syntax:
 * @code
 *   r = BITOR(a, b)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return (a | b) as int64_t.
 */
int64_t basic_bitor(struct basic_ctx* ctx);

/**
 * @brief Implements BITAND(a, b) — bitwise AND.
 *
 * BASIC syntax:
 * @code
 *   r = BITAND(a, b)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return (a & b) as int64_t.
 */
int64_t basic_bitand(struct basic_ctx* ctx);

/**
 * @brief Implements BITNOT(a) — bitwise complement.
 *
 * BASIC syntax:
 * @code
 *   r = BITNOT(a)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return (~a) as int64_t.
 *
 * @note Two’s-complement semantics on 64-bit integers.
 */
int64_t basic_bitnot(struct basic_ctx* ctx);

/**
 * @brief Implements BITEOR(a, b) — bitwise XOR (BBC EOR).
 *
 * BASIC syntax:
 * @code
 *   r = BITEOR(a, b)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return (a ^ b) as int64_t.
 */
int64_t basic_biteor(struct basic_ctx* ctx);

/**
 * @brief Implements BITNAND(a, b) — bitwise NAND.
 *
 * BASIC syntax:
 * @code
 *   r = BITNAND(a, b)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return ~(a & b) as int64_t.
 */
int64_t basic_bitnand(struct basic_ctx* ctx);

/**
 * @brief Implements BITNOR(a, b) — bitwise NOR.
 *
 * BASIC syntax:
 * @code
 *   r = BITNOR(a, b)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return ~(a | b) as int64_t.
 */
int64_t basic_bitnor(struct basic_ctx* ctx);

/**
 * @brief Implements BITXNOR(a, b) — bitwise equivalence.
 *
 * BASIC syntax:
 * @code
 *   r = BITXNOR(a, b)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return ~(a ^ b) as int64_t.
 */
int64_t basic_bitxnor(struct basic_ctx* ctx);

/**
 * @brief Implements BITSHL(a, n) — logical left shift.
 *
 * BASIC syntax:
 * @code
 *   r = BITSHL(a, n)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return a << n, computed in 64-bit.
 *
 * @note Implementations typically clamp n to 0–63.
 */
int64_t basic_bitshl(struct basic_ctx* ctx);

/**
 * @brief Implements BITSHR(a, n) — logical right shift.
 *
 * BASIC syntax:
 * @code
 *   r = BITSHR(a, n)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return Logical shift right of a by n (uses unsigned semantics), widened to int64_t.
 *
 * @note Implementations typically clamp n to 0–63.
 */
int64_t basic_bitshr(struct basic_ctx* ctx);

/**
 * @brief Implements BITROL(a, n, width) — rotate left within width bits.
 *
 * BASIC syntax:
 * @code
 *   r = BITROL(a, n, width)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return a rotated left by n within the lowest @p width bits.
 *
 * @note Width is usually 1–64; the result is masked to @p width bits.
 */
int64_t basic_bitrol(struct basic_ctx* ctx);

/**
 * @brief Implements BITROR(a, n, width) — rotate right within width bits.
 *
 * BASIC syntax:
 * @code
 *   r = BITROR(a, n, width)
 * @endcode
 *
 * @param ctx Interpreter context.
 * @return a rotated right by n within the lowest @p width bits.
 *
 * @note Width is usually 1–64; the result is masked to @p width bits.
 */
int64_t basic_bitror(struct basic_ctx* ctx);

void memrelease_statement(struct basic_ctx* ctx);

int64_t basic_memalloc(struct basic_ctx* ctx);