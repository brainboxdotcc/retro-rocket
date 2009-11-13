#ifndef __TASKSWITCH_H__
#define __TASKSWITCH_H__

#include "kernel.h"
#include "paging.h"

// This structure defines a 'task' - a process.
typedef struct task
{
	int id;			// Process ID.
	u32int esp, ebp;	// Stack and base pointers.
	u32int eip;		// Instruction pointer.
	u8int supervisor;	// Is a supervisor (kernel mode) task
	page_directory_t *page_directory; // Page directory.
	struct task *next;	// The next task in a linked list.
} task_t;

// A struct describing a Task State Segment.
struct tss_entry_struct
{
	u32int prev_tss;		// The previous TSS - if we used hardware task switching this would form a linked list.
	u32int esp0;			// The stack pointer to load when we change to kernel mode.
	u32int ss0;			// The stack segment to load when we change to kernel mode.
	u32int esp1;			// Unused...
	u32int ss1;
	u32int esp2;
	u32int ss2;
	u32int cr3;
	u32int eip;
	u32int eflags;
	u32int eax;
	u32int ecx;
	u32int edx;
	u32int ebx;
	u32int esp;
	u32int ebp;
	u32int esi;
	u32int edi;
	u32int es;			// The value to load into ES when we change to kernel mode.
	u32int cs;			// The value to load into CS when we change to kernel mode.
	u32int ss;			// The value to load into SS when we change to kernel mode.
	u32int ds;			// The value to load into DS when we change to kernel mode.
	u32int fs;			// The value to load into FS when we change to kernel mode.
	u32int gs;			// The value to load into GS when we change to kernel mode.
	u32int ldt;			// Unused...
	u16int trap;
	u16int iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

// Initialises the tasking system.
void initialise_tasking();

// Called by the timer hook, this changes the running process.
void switch_task();

// Forks the current process, spawning a new one with a different
// memory space.
int fork(u8int supervisor);

// Causes the current process' stack to be forcibly moved to a new location.
void move_stack(void *new_stack_start, u32int size);

// Returns the pid of the current process.
int getpid();

void write_tss(s32int num, u16int ss0, u32int esp0);

void set_kernel_stack(u32int stack);

#endif
