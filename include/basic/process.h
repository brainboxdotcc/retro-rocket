#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Get the total number of processes.
 *
 * This function retrieves the total number of processes currently in the system.
 *
 * @param ctx The BASIC context.
 * @return The total number of processes.
 */
int64_t basic_getproccount(struct basic_ctx* ctx);

/**
 * @brief Get the amount of free memory in the system.
 *
 * This function retrieves the amount of free memory currently available in the system.
 *
 * @param ctx The BASIC context.
 * @return The amount of free memory in bytes.
 */
int64_t basic_get_free_mem(struct basic_ctx* ctx);

/**
 * @brief Get the amount of memory in use by the system.
 *
 * This function retrieves the amount of memory that is currently in use by the system.
 *
 * @param ctx The BASIC context.
 * @return The amount of used memory in bytes.
 */
int64_t basic_get_used_mem(struct basic_ctx* ctx);

/**
 * @brief Get the total amount of memory in the system.
 *
 * This function retrieves the total amount of memory in the system.
 *
 * @param ctx The BASIC context.
 * @return The total amount of memory in bytes.
 */
int64_t basic_get_total_mem(struct basic_ctx* ctx);

/**
 * @brief Get the ID of a process.
 *
 * This function retrieves the process ID of a specified process.
 *
 * @param ctx The BASIC context.
 * @return The process ID of the specified process.
 */
int64_t basic_getprocid(struct basic_ctx* ctx);

/**
 * @brief Get the name of a process.
 *
 * This function retrieves the name of the process identified by the specified process ID.
 *
 * @param ctx The BASIC context.
 * @return The name of the specified process.
 */
char* basic_getprocname(struct basic_ctx* ctx);

/**
 * @brief Get the parent process ID of a specified process.
 *
 * This function retrieves the parent process ID of a specified process.
 *
 * @param ctx The BASIC context.
 * @return The parent process ID of the specified process.
 */
int64_t basic_getprocparent(struct basic_ctx* ctx);

/**
 * @brief Get the CPU ID of the process.
 *
 * This function retrieves the CPU ID of the specified process.
 *
 * @param ctx The BASIC context.
 * @return The CPU ID of the specified process.
 */
int64_t basic_getproccpuid(struct basic_ctx* ctx);

/**
 * @brief Get the memory usage of the process.
 *
 * This function retrieves the amount of memory used by the process identified by the process ID.
 *
 * @param ctx The BASIC context.
 * @return The memory usage of the specified process in bytes.
 */
int64_t basic_getprocmem(struct basic_ctx* ctx);

/**
 * @brief Get the peak memory usage of the program.
 *
 * This function retrieves the peak memory usage of the program running within the BASIC environment.
 *
 * @param ctx The BASIC context.
 * @return The peak memory usage of the program in bytes.
 */
int64_t basic_get_program_peak_mem(struct basic_ctx* ctx);

/**
 * @brief Get the current memory usage of the program.
 *
 * This function retrieves the current memory usage of the program running within the BASIC environment.
 *
 * @param ctx The BASIC context.
 * @return The current memory usage of the program in bytes.
 */
int64_t basic_get_program_cur_mem(struct basic_ctx* ctx);

/**
 * @brief Get the state of a specified process.
 *
 * This function retrieves the current state of the specified process, such as running, suspended, or waiting.
 *
 * @param ctx The BASIC context.
 * @return The state of the process (e.g., "running", "suspended", "waiting").
 */
char* basic_getprocstate(struct basic_ctx* ctx);
