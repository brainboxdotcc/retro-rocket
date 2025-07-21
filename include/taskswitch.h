/**
 * @file taskswitch.h
 * @brief Handles processes and multitasking
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

/**
 * @brief Represents the current state of a process
 */
typedef enum process_state_t {
	PROC_RUNNING,
	PROC_IDLE,
	PROC_DELET,
} process_state_t;

typedef uint32_t pid_t;		// Process ID
typedef uint32_t uid_t;		// User ID
typedef uint32_t gid_t;		// Group ID
typedef uint8_t cpu_id_t;	// CPU ID

typedef struct process_t {
	pid_t			pid;		/* Process ID */
	pid_t			ppid;		/* Parent Process ID */
	uid_t			uid;		/* User id - Future use */
	gid_t			gid;		/* Group id - Future use */
	process_state_t		state;		/* Running state */
	time_t			start_time;	/* Start time (UNIX epoch)*/
	pid_t			waitpid;	/* PID we are waiting on compltion of */
	cpu_id_t		cpu;		/* CPU ID */
	const char*		directory;	/* Directory of program */
	const char*		name;		/* Filename of program */
	uint64_t		size;		/* Size of program in bytes */
	const char*		csd;		/* Current selected directory */
	struct console*		cons;		/* Program's console */
	struct basic_ctx*	code;		/* BASIC context */
	struct process_t*	prev;		/* Prev process in doubly linked list */
	struct process_t*	next;		/* Next process in doubly linked list */
} process_t;

/**
 * @brief Process identifier struct.
 * Used to identify processes by ID in the hash map only,
 * contains the id and a pointer to the actual process_t struct
 */
typedef struct proc_id_t {
	uint32_t id;		// Process ID
	process_t* proc;	// Process detail
} proc_id_t;

/**
 * @brief Types of idle task
 */
typedef enum idle_type_t {
	IDLE_FOREGROUND, // A foreground idle task that runs in the task switch loop between context switches
	IDLE_BACKGROUND, // A background idle task that runs via the LAPIC timer ISR
} idle_type_t;

// Timer that drivers may register to be called during the idle time
typedef void (*proc_idle_timer_t)(void);

/**
 * @brief An idle timer
 */
typedef struct idle_timer {
	proc_idle_timer_t func;
	struct idle_timer* next;
} idle_timer_t;

/**
 * @brief Load a new BASIC process
 * 
 * @param fullpath fully qualified path to file
 * @param cons console
 * @param parent_pid parent PID, or 0
 * @param csd Currently selected directory
 * @return process_t* new process details
 */
process_t* proc_load(const char* fullpath, struct console* cons, pid_t parent_pid, const char* csd);

/**
 * @brief Find a process by ID
 * 
 * @param pid process ID
 * @return process_t* process detail or NULL if not found
 */
process_t* proc_find(pid_t pid);

/**
 * @brief Return detail of current process
 * 
 * @return process_t* process detail or NULL if no current process
 */
process_t* proc_cur();

/**
 * @brief Mark a process as waiting for another process to complete
 * 
 * @param proc process to mark as waiting
 * @param otherpid other process ID to wait on, must exist.
 */
void proc_wait(process_t* proc, pid_t otherpid);

/**
 * @brief Run BASIC program for one atomic cycle
 * 
 * @param proc process to execute
 */
void proc_run(process_t* proc);

/**
 * @brief Returns true if the program has ended
 * 
 * @param proc process
 * @return int true if ended
 */
int proc_ended(process_t* proc);

/**
 * @brief Kill a process
 * 
 * @param proc process to kill
 */
void proc_kill(process_t* proc);

/**
 * @brief Kill a process by ID
 * 
 * @note Cannot be used to kill the current process from itself!
 * @param id Process ID to kill
 * @return true if found and killed
 */
bool proc_kill_id(pid_t id);

/**
 * @brief Display a diagnostic list of all processes
 */
void proc_show_list();

/**
 * @brief Run the process scheduling loop.
 * @note Does not return
 */
_Noreturn void proc_loop();

/**
 * @brief Change to next scheduled process
 * @note Uses the round robin scheduling algorithm
 */
void proc_timer();

/**
 * @brief Returns the total number of running processes
 * 
 * @return int64_t number of running processes
 */
int64_t proc_total();

/**
 * @brief Returns the id of a process by index number
 * 
 * @param index index number of process to find between 0 and proc_total()
 * @return pid_t process id
 */
pid_t proc_id(int64_t index);

/**
 * @brief Register a function to be called periodically during idle time
 * 
 * @param handler handler function, void(void)
 * @param type type of idle to register
 */
void proc_register_idle(proc_idle_timer_t handler, idle_type_t type);

/**
 * @brief Change CSD (currently selected directory) of process
 * 
 * @note No validation of the path is peformed, this must be done
 * extnerally to this function by validating the file information on VFS.
 * @param proc Process struct
 * @param csd current directory
 * @return const char* new current directory
 */
const char* proc_set_csd(process_t* proc, const char* csd);

void init_process();

