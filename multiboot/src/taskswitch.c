#include "../include/kernel.h"
#include "../include/taskswitch.h"
#include "../include/paging.h"
#include "../include/filesystem.h"
#include "../include/interrupts.h"
#include "../include/syscall.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"
#include "../include/kprintf.h"

struct process* proc_current = NULL;
struct process* proc_list = NULL;

u32int nextid = 1;

struct process* proc_load(const char* fullpath, struct console* cons)
{
	FS_DirectoryEntry* fsi = fs_get_file_info(fullpath);
	if (fsi != NULL)
	{
		unsigned char* programtext = (unsigned char*)kmalloc(fsi->size + 1);
		*(programtext + fsi->size) = 0;
		if (fs_read_file(fsi, 0, fsi->size, programtext))
		{
			//kprintf("program len = %d size = %d\n", strlen(programtext), fsi->size);
			struct process* newproc = (struct process*)kmalloc(sizeof(struct process));
			newproc->code = ubasic_init((const char*)programtext, (console*)cons);
			newproc->name = strdup(fsi->filename);
			newproc->pid = nextid++;
			newproc->size = fsi->size;
			newproc->cons = cons;
			newproc->text = programtext;

			interrupts_off();

			if (proc_list == NULL)
			{
				/* First process */
				proc_list = newproc;
				newproc->next = NULL;
				newproc->prev = NULL;
			}
			else
			{
				/* Any other process */
				newproc->next = proc_list;
				newproc->prev = NULL;
				proc_list->prev = newproc;
				proc_list = newproc;
			}

			// No current proc? Make it the only proc.
			if (proc_current == NULL)
				proc_current = proc_list;

			interrupts_on();

			return newproc;
		}
		else
		{
			kfree(programtext);
		}

		return NULL;
	}

	return NULL;
}

void proc_run(struct process* proc)
{
	ubasic_run(proc->code);
}

void proc_kill(struct process* proc)
{
	//kprintf("prog: '%s'\n", proc->code->program_ptr);
	kprintf("proc_kill\n");
	interrupts_off();
	struct process* cur = proc_list;
	
	// TOdO: If process being killed is current process, move to another process.
	if (proc_current->next != NULL)
		proc_current = proc_current->next;
	else if (proc_current->prev != NULL)
		proc_current = proc_current->prev;
	else
		proc_current = NULL;

	for (; cur; cur = cur->next)
	{
		if (cur->pid == proc->pid)
		{
			struct process* next = proc->next;
			struct process* prev = proc->prev;

			/* Remove this process from the linked list */
			prev->next = next;
			next->prev = prev;
		}
	}
	ubasic_destroy(proc->code);
	kfree(proc->name);
	kfree(proc->text);
	interrupts_on();
}

void proc_show_list()
{
	interrupts_off();
	struct process* cur = proc_list;
	for (; cur; cur = cur->next)
	{
		kprintf("PID %d name %s\n", cur->pid, cur->name);
	}
	interrupts_on();
}

void proc_run_next()
{
	struct process* current;
	interrupts_off();
	current = proc_current;
	interrupts_on();

	//kprintf("*");

	if (current != NULL)
	{
		//kprintf("?");
		proc_run(current);
		//kprintf("!");
		if (proc_ended(current))
		{
			//kprintf("$");
			proc_kill(current);
			//kprintf("@");
		}
		//kprintf("~");
	}
	//kprintf("%%");
}

void proc_loop()
{
	while (1)
	{
		proc_run_next();

		/* Idle till next timer interrupt */
		//asm volatile("hlt");
	}
}

void proc_timer()
{
	if (proc_list == NULL)
		return;

	if (proc_current->next == NULL)
		proc_current = proc_list;
	else
		proc_current = proc_current->next;
	//kprintf("%08x", proc_current);
}

int proc_ended(struct process* proc)
{
	int r = ubasic_finished(proc->code);
	return r;
}

