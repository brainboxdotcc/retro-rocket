#include <kernel.h>
#include <taskswitch.h>
#include <paging.h>
#include <filesystem.h>
#include <interrupts.h>
#include <syscall.h>
#include <string.h>
#include <kmalloc.h>
#include <memcpy.h>
#include <kprintf.h>

struct process* proc_current = NULL;	/* This can be changed by an interrupt, MAKE A COPY */
struct process* proc_list = NULL;
static struct process* proc_running = NULL;	/* This is the safe copy of proc_current */

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
			newproc->waitpid = 0;
			newproc->name = strdup(fsi->filename);
			newproc->pid = nextid++;
			newproc->size = fsi->size;
			newproc->cons = cons;
			//newproc->text = programtext;
			kfree(programtext);

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

struct process* proc_cur()
{
	return proc_running;
}

void proc_run(struct process* proc)
{
	proc_running = proc;
	if (proc->waitpid == 0)
		ubasic_run(proc->code);
	else if (proc_find(proc->waitpid) == NULL)
	{
		//kprintf("Process %d exited, resuming %d\n", proc->waitpid, proc->pid);
		proc->waitpid = 0;
		ubasic_run(proc->code);
	}
	else
		asm volatile("hlt");
	//else
	//	kprintf("proc_find(%d) == %d\n.\n", proc->waitpid, proc_find(proc->waitpid));
}

struct process* proc_find(u32int pid)
{
	interrupts_off();
	struct process* cur = proc_list;
	struct process* foundproc = NULL;
	for (; cur; cur = cur->next)
		if (cur->pid == pid)
			foundproc = cur;
	interrupts_on();

	return foundproc;
}

void proc_wait(struct process* proc, u32int otherpid)
{
	//kprintf("Process waiting for pid %d\n", otherpid);
	proc->waitpid = otherpid;
}

void proc_kill(struct process* proc)
{
	//kprintf("prog: '%s'\n", proc->code->program_ptr);
	//kprintf("proc_kill %d\n", proc->pid);
	
	interrupts_off();
	struct process* cur = proc_list;

	//kprintf("'%s'\n", proc->code->program_ptr);
	
	int countprocs = 0;
	for (; cur; cur = cur->next)
	{
		countprocs++;
		if (cur->pid == proc->pid)
		{
			if (proc->next == NULL && proc->prev == NULL)
			{
				// the only process!
				proc_list = NULL;
			}
			else if (proc->prev == NULL && proc->next != NULL)
			{
				// first item
				proc_list = proc->next;
				proc_list->prev = NULL;
			}
			else if (proc->prev != NULL && proc->next == NULL)
			{
				// last item
				proc->prev->next = NULL;
			}
			else
			{
				// middle item
				proc->prev->next = proc->next;
				proc->next->prev = proc->prev;
			}

			break;
		}
	}

	proc_current = proc_list;

	ubasic_destroy(proc->code);
	kfree(proc->name);
	//kfree(proc->text);

	/* milled the last process! */
	if (proc_list == NULL)
	{
		kprintf("\nSystem halted.");
		blitconsole(current_console);
		wait_forever();
	}

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
		//kprintf(".");
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

