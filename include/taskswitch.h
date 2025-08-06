/**
 * @file taskswitch.h
 * @brief Handles process management, multitasking, and scheduling
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include <kernel.h>

/**
 * @brief Represents the current state of a process.
 */
typedef enum process_state_t {
	PROC_RUNNING,	/**< Process is currently running */
	PROC_SUSPENDED,	/**< Process is waiting on another process */
	PROC_IO_BOUND,  /**< Process is waiting on IO */
} process_state_t;

/**
 * @brief Process ID type (unique identifier for a process).
 */
typedef uint32_t pid_t;

/**
 * @brief User ID type (reserved for future use).
 */
typedef uint32_t uid_t;

/**
 * @brief Group ID type (reserved for future use).
 */
typedef uint32_t gid_t;

/**
 * @brief Logical CPU ID type.
 */
typedef uint8_t cpu_id_t;

struct process_t;

/**
 * @typedef activity_callback_t
 * @brief Callback to determine if a process is still idle.
 *
 * This function is invoked by the scheduler to decide whether
 * a process should remain suspended (idle) or be resumed.
 *
 * Return `true` if the process should remain idle,
 * `false` if it is ready to run again.
 *
 * @param proc Pointer to the process being evaluated
 * @param opaque For end user use
 * @return bool true if idle; false if active
 */
typedef bool (*activity_callback_t)(struct process_t* proc, void* opaque);

/**
 * @brief Represents a process in the system.
 *
 * Each process has its own PID, optional parent PID, execution state,
 * associated console, and BASIC execution context. Processes are linked
 * in a global doubly-linked list for scheduling.
 */
typedef struct process_t {
	pid_t			pid;        /**< Unique process ID */
	pid_t			ppid;       /**< Parent process ID */
	uid_t			uid;        /**< User ID (future use) */
	gid_t			gid;        /**< Group ID (future use) */
	process_state_t		state;      /**< Running state */
	time_t			start_time; /**< Start time (UNIX epoch) */
	pid_t			waitpid;    /**< PID being waited on */
	cpu_id_t		cpu;        /**< Logical CPU this process is assigned to */
	const char*		directory;  /**< Directory of program */
	const char*		name;       /**< Filename of program */
	uint64_t		size;       /**< Size of program in bytes */
	const char*		csd;        /**< Current selected directory */
	struct console*		cons;       /**< Associated console */
	struct basic_ctx*	code;       /**< BASIC interpreter context */
	struct process_t*	prev;       /**< Previous process in doubly linked list */
	struct process_t*	next;       /**< Next process in doubly linked list */
	activity_callback_t	check_idle; /**< If non-null, called to check if the process should remain idle */
	void*			idle_context; /**< Opaque context passed to the check_idle callback */
} process_t;

/**
 * @brief Process identifier struct.
 *
 * Used only in the process hash map for quick lookup by ID.
 */
typedef struct proc_id_t {
	uint32_t id;       /**< Process ID */
	process_t* proc;   /**< Pointer to process detail */
} proc_id_t;

/**
 * @brief Types of idle task.
 */
typedef enum idle_type_t {
	IDLE_FOREGROUND, /**< Idle task running in main task loop between switches */
	IDLE_BACKGROUND, /**< Idle task running via LAPIC timer ISR */
} idle_type_t;

/**
 * @brief Function pointer type for an idle timer callback.
 *
 * Functions registered here are periodically invoked
 * during idle time, depending on idle type.
 */
typedef void (*proc_idle_timer_t)(void);

/**
 * @brief Represents an idle timer callback.
 */
typedef struct idle_timer {
	proc_idle_timer_t func;       /**< Function pointer for callback */
	struct idle_timer* next;      /**< Next idle timer in list */
} idle_timer_t;

/**
 * @brief Load and start a new BASIC process.
 *
 * @param fullpath Fully qualified path to file
 * @param cons Associated console
 * @param parent_pid Parent PID, or 0 for none
 * @param csd Current selected directory
 * @return process_t* Pointer to new process details
 */
process_t* proc_load(const char* fullpath, struct console* cons, pid_t parent_pid, const char* csd);

/**
 * @brief Find a process by PID.
 *
 * @param pid Process ID
 * @return process_t* Pointer to process detail or NULL if not found
 */
process_t* proc_find(pid_t pid);

/**
 * @brief Get current process for a logical CPU.
 *
 * @param logical_cpu CPU ID
 * @return process_t* Current process or NULL if none
 */
process_t* proc_cur(uint8_t logical_cpu);

/**
 * @brief Mark a process as waiting for another to complete.
 *
 * @param proc Process to mark as waiting
 * @param otherpid PID to wait on (must exist)
 */
void proc_wait(process_t* proc, pid_t otherpid);

/**
 * @brief Execute one atomic BASIC cycle for a process.
 *
 * @param proc Process to run
 */
void proc_run(process_t* proc);

/**
 * @brief Determine if a program has ended.
 *
 * @param proc Process to check
 * @return int Non-zero if ended, zero otherwise
 */
int proc_ended(process_t* proc);

/**
 * @brief Kill a process immediately.
 *
 * @param proc Process to terminate
 */
void proc_kill(process_t* proc);

/**
 * @brief Kill a process by PID.
 *
 * @note Cannot be used to kill the current process from itself.
 * @param id Process ID
 * @return true if process found and killed, false otherwise
 */
bool proc_kill_id(pid_t id);

/**
 * @brief Run the process scheduling loop.
 *
 * Each logical CPU has its own scheduling loop.
 * @note This function does not return.
 */
_Noreturn void proc_loop();

/**
 * @brief Switch to the next scheduled process.
 *
 * Implements a round-robin scheduling algorithm.
 */
void proc_timer();

/**
 * @brief Get total number of running processes.
 *
 * @return int64_t Number of running processes
 */
int64_t proc_total();

/**
 * @brief Get the PID of a process by index.
 *
 * @param index Process index, between 0 and proc_total()
 * @return pid_t Process ID
 */
pid_t proc_id(int64_t index);

/**
 * @brief Register an idle callback function.
 *
 * @param handler Function pointer, void(void)
 * @param type Foreground or background idle type
 */
void proc_register_idle(proc_idle_timer_t handler, idle_type_t type);

/**
 * @brief Change the CSD (current selected directory) of a process.
 *
 * @note No validation of the path is performed here; external VFS
 *       checks must be applied before calling.
 * @param proc Process to update
 * @param csd New current directory
 * @return const char* Updated current directory
 */
const char* proc_set_csd(process_t* proc, const char* csd);

/**
 * @brief Initialise the process subsystem.
 */
void init_process();

/**
 * @brief Set or clear the idle state of a process.
 *
 * This sets a callback that determines whether the process is ready to resume.
 * When a callback is provided, the process will be marked as suspended and
 * polled each scheduling round. If the callback returns false, the process
 * will be resumed. Passing NULL clears the idle state immediately.
 *
 * @param proc Process to update
 * @param callback Function to poll for resumption readiness, or NULL to resume immediately
 * @param opaque For use by developer
 */
void proc_set_idle(process_t* proc, activity_callback_t callback, void* opaque);