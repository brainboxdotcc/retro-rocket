#include "../include/kernel.h"
#include "../include/taskswitch.h"
#include "../include/interrupts.h"
#include "../include/kmalloc.h"
#include "../include/printf.h"
#include "../include/paging.h"
#include "../include/memcpy.h"
#include "../include/io.h"

// The currently running task.
volatile task_t *current_task;

// The start of the task linked list.
volatile task_t *ready_queue;

// Some externs are needed to access members in paging.c...
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern void alloc_frame(page_t*,int,int);
extern u32int initial_esp;
extern u32int read_eip();
extern void tss_flush();

tss_entry_t tss_entry;

// The next available process ID.
u32int next_pid = 1;

void initialise_tasking()
{
	// Rather important stuff happening, no interrupts please!
	asm volatile("cli");

	// Relocate the stack so we know where it is.
	move_stack((void*)0xE0000000, 0x2000);

	// Initialise the first task (kernel task)
	current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->supervisor = 1;
	current_task->page_directory = current_directory;
	current_task->next = 0;

	// Reenable interrupts.
	asm volatile("sti");
}

void move_stack(void *new_stack_start, u32int size)
{
	u32int i;
	// Allocate some space for the new stack.
	for( i = (u32int)new_stack_start; i >= ((u32int)new_stack_start-size); i -= 0x1000)
	{
		// General-purpose stack is in user-mode.
		alloc_frame( get_page(i, 1, current_directory), 0 /* User mode */, 1 /* Is writable */ );
	}
	
	// Flush the TLB by reading and writing the page directory address again.
	u32int pd_addr;
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

	// Old ESP and EBP, read from registers.
	u32int old_stack_pointer;
	asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
	u32int old_base_pointer;
	asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer));

	// Offset to add to old stack addresses to get a new stack address.
	u32int offset = (u32int)new_stack_start - initial_esp;

	// New ESP and EBP.
	u32int new_stack_pointer = old_stack_pointer + offset;
	u32int new_base_pointer	= old_base_pointer	+ offset;

	// Copy the stack.
	memcpy((void*)new_stack_pointer, (void*)old_stack_pointer, initial_esp-old_stack_pointer);

	// Backtrace through the original stack, copying new values into
	// the new stack.	
	for(i = (u32int)new_stack_start; i > (u32int)new_stack_start-size; i -= 4)
	{
		u32int tmp = * (u32int*)i;
		// If the value of tmp is inside the range of the old stack, assume it is a base pointer
		// and remap it. This will unfortunately remap ANY value in this range, whether they are
		// base pointers or not.
		if (( old_stack_pointer < tmp) && (tmp < initial_esp))
		{
			tmp = tmp + offset;
			u32int *tmp2 = (u32int*)i;
			*tmp2 = tmp;
		}
	}

	// Change stacks.
	asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
	asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}

void switch_task()
{
	// If we haven't initialised tasking yet, just return.
	if (!current_task)
		return;

	// Read esp, ebp now for saving later on.
	u32int esp, ebp, eip;
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));

	// Read the instruction pointer. We do some cunning logic here:
	// One of two things could have happened when this function exits - 
	// (a) We called the function and it returned the EIP as requested.
	// (b) We have just switched tasks, and because the saved EIP is essentially
	//	  the instruction after read_eip(), it will seem as if read_eip has just
	//	  returned.
	// In the second case we need to return immediately. To detect it we put a dummy
	// value in EAX further down at the end of this function. As C returns values in EAX,
	// it will look like the return value is this dummy value! (0x12345).
	eip = read_eip();

	// Have we just switched tasks?
	if (eip == 0x12345)
		return;

	// No, we didn't switch tasks. Let's save some register values and switch.
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;
	
	// Get the next task to run.
	current_task = current_task->next;
	// If we fell off the end of the linked list start again at the beginning.
	if (!current_task) current_task = ready_queue;

	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;

	// Make sure the memory manager knows we've changed page directory.
	current_directory = current_task->page_directory;
	// Here we:
	// * Stop interrupts so we don't get interrupted.
	// * Temporarily puts the new EIP location in ECX.
	// * Loads the stack and base pointers from the new task struct.
	// * Changes page directory to the physical address (physicalAddr) of the new directory.
	// * Puts a dummy value (0x12345) in EAX so that above we can recognise that we've just
	//	switched task.
	// * Restarts interrupts. The STI instruction has a delay - it doesn't take effect until after
	//	the next instruction.
	// * Jumps to the location in ECX (remember we put the new EIP in there).
	//
	// XXX: We must remember to switch stack for usermode?
	asm volatile("			\
		cli;			\
		mov %0, %%ecx;		\
		mov %1, %%esp;		\
		mov %2, %%ebp;		\
		cmp 0, %4;		\
		je usermode;		\
	supervisormode:			\
		mov %3, %%cr3; 		\
		mov $0x12345, %%eax;	\
		sti;			\
		jmp *%%ecx;		\
	usermode:			\
		mov $0x23, %%ax;	\
		mov %%ax, %%ds;		\
		mov %%ax, %%es;		\
		mov %%ax, %%fs; 	\
		mov %%ax, %%gs; 	\
		mov %%esp, %%eax; 	\
		pushl $0x23; 		\
		pushl %%eax; 		\
		pushf; 			\
		pop %%eax; 		\
		or $0x200, %%eax; 	\
		push %%eax;		\
		pushl $0x1B;		\
		push %%ecx;		\
		mov %3, %%cr3;		\
		iret;			\
		"
		 : : "r"(eip), "r"(esp), "r"(ebp), "r"(current_directory->physicalAddr), "a"(current_task->supervisor));
}

// Create process steps:
// (1) sanity check ELF file, grab some pages and load elf into them
// (2) call fork() below, with a (yet to be added) supervisor parameter
// (3) in PARENT process, we can get rid of the ELF pages?
// (4) loop the new thread until we are in usermode (we can check this by looking
// at our selector etc)
// (5) once in usermode call the entrypoint of the executable
// (6) if it exits, we have to handle thread termination...

int fork(u8int supervisor)
{
	// We are modifying kernel structures, and so cannot
	asm volatile("cli");

	// Take a pointer to this process' task struct for later reference.
	// XXX: If the parent exits this needs to be amended. Right now there
	// is no facility for threads/processes to exit.
	task_t *parent_task = (task_t*)current_task;

	// Clone the address space.
	page_directory_t *directory = clone_directory(current_directory);

	// Create a new process.
	task_t *new_task = (task_t*)kmalloc(sizeof(task_t));

	new_task->id = next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->supervisor = supervisor;
	new_task->page_directory = directory;
	new_task->next = 0;

	// Add it to the end of the ready queue.
	task_t *tmp_task = (task_t*)ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;
	tmp_task->next = new_task;

	// This will be the entry point for the new process.
	u32int eip = read_eip();


	// We could be the parent or the child here - check.
	if (current_task == parent_task)
	{
		// We are the parent, so set up the esp/ebp/eip for our child.
		u32int esp; asm volatile("mov %%esp, %0" : "=r"(esp));
		u32int ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;
		asm volatile("sti");

		return new_task->id;
	}
	else
	{
		// We are the child process
		// XXX: Child process should drop to usermode. 
		// XXX: See switch_task() and comments above
		outb(0x20, 0x20); /*end of interrupt */
		return 0;
	}

}

int getpid()
{
	return current_task->id;
}

void set_kernel_stack(u32int stack)
{
	   tss_entry.esp0 = stack;
}

// Initialise our task state segment structure.
void write_tss(s32int num, u16int ss0, u32int esp0)
{
	// Firstly, let's compute the base and limit of our entry into the GDT.
	u32int base = (u32int) &tss_entry;
	u32int limit = base + sizeof(tss_entry);

	// Now, add our TSS descriptor's address to the GDT.
	gdt_set_gate(num, base, limit, 0xE9, 0x00);

	// Ensure the descriptor is initially zero.
	_memset(&tss_entry, 0, sizeof(tss_entry));

	tss_entry.ss0  = ss0;  // Set the kernel stack segment.
	tss_entry.esp0 = esp0; // Set the kernel stack pointer.

	// Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify what
	// segments should be loaded when the processor switches to kernel mode. Therefore
	// they are just our normal kernel code/data segments - 0x08 and 0x10 respectively,
	// but with the last two bits set, making 0x0b and 0x13. The setting of these bits
	// sets the RPL (requested privilege level) to 3, meaning that this TSS can be used
	// to switch to kernel mode from ring 3.
	tss_entry.cs	= 0x0b;
	tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
} 

