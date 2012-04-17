#include <kernel.h>

struct process* proc_current[256] = { NULL };	/* This can be changed by an interrupt, MAKE A COPY */
struct process* proc_list = NULL;
static struct process* proc_running = NULL;	/* This is the safe copy of proc_current */

u32 nextid = 1;
spinlock schedlock = 0;

struct process* proc_load(const char* fullpath, struct console* cons)
{
	FS_DirectoryEntry* fsi = fs_get_file_info(fullpath);
	if (fsi != NULL)
	{
		unsigned char* programtext = (unsigned char*)kmalloc(fsi->size + 1);
		*(programtext + fsi->size) = 0;
		if (fs_read_file(fsi, 0, fsi->size, programtext))
		{
			struct process* newproc = (struct process*)kmalloc(sizeof(struct process));
			newproc->code = ubasic_init((const char*)programtext, (console*)cons);
			newproc->waitpid = 0;
			newproc->name = strdup(fsi->filename);
			newproc->pid = nextid++;
			newproc->size = fsi->size;
			newproc->cons = cons;
			newproc->state = PROC_IDLE;
			newproc->ticks = 0;
			init_spinlock(&newproc->lock);
			kfree(programtext);

			lock_spinlock(&schedlock);

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
			//if (proc_current == NULL)
			//	proc_current[cpu_id()] = proc_list;

			unlock_spinlock(&schedlock);

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
	return proc_current[cpu_id()];
}

void proc_run(struct process* proc)
{
	//kprintf("proc_run\n");
	lock_spinlock(&proc->lock);
	if (proc->waitpid == 0)
	{
		ubasic_run(proc->code);
	}
	else if (proc_find(proc->waitpid) == NULL)
	{
		proc->waitpid = 0;
		ubasic_run(proc->code);
	}
	proc->ticks++;
	unlock_spinlock(&proc->lock);
}

struct process* proc_find(u32 pid)
{
	lock_spinlock(&schedlock);
	struct process* cur = proc_list;
	struct process* foundproc = NULL;
	for (; cur; cur = cur->next)
		if (cur->pid == pid)
			foundproc = cur;
	unlock_spinlock(&schedlock);

	return foundproc;
}

void proc_wait(struct process* proc, u32 otherpid)
{
	//kprintf("Process waiting for pid %d\n", otherpid);
	proc->waitpid = otherpid;
}

void proc_kill(struct process* proc)
{
	//kprintf("prog: '%s'\n", proc->code->program_ptr);
	//kprintf("proc_kill %d\n", proc->pid);
	
	lock_spinlock(&schedlock);
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

	proc_current[cpu_id()] = NULL;

	ubasic_destroy(proc->code);
	kfree(proc->name);

	/* Killed the last process! */
	if (proc_list == NULL)
	{
		//kprintf("\nSystem halted.");
		blitconsole(current_console);
		wait_forever();
	}

	unlock_spinlock(&schedlock);
}

void proc_show_list()
{
	lock_spinlock(&schedlock);
	struct process* cur = proc_list;
	for (; cur; cur = cur->next)
	{
		kprintf("PID %d name %s\n", cur->pid, cur->name);
	}
	unlock_spinlock(&schedlock);
}

void proc_run_next()
{
	struct process* current;
	lock_spinlock(&schedlock);
	current = proc_current[cpu_id()];
	unlock_spinlock(&schedlock);

	if (current != NULL)
	{
		proc_run(current);
		lock_spinlock(&schedlock);
		current->state = PROC_IDLE;
		unlock_spinlock(&schedlock);
		if (proc_ended(current))
			proc_kill(current);
	}
	//else
	//	kprintf("current == null\n");
}

void proc_loop()
{
	while (1)
	{
		proc_timer();
		proc_run_next();
		//asm volatile("hlt");
	}
}

/* Find the first idle process and set it to RUNNING */
struct process* proc_first_idle()
{
	struct process* cur = proc_list;
	while (cur)
	{
		if (cur->state == PROC_IDLE)
		{
			//kprintf("first idle %08x\n", cur);
			cur->state = PROC_RUNNING;
			return cur;
		}
		else if (cur->state == PROC_RUNNING)
		{
			if (cur->ticks > 1)
			{
				cur->state = PROC_IDLE;
				cur->ticks = 0;
			}
		}
		cur = cur->next;
	}
	return NULL;
}

void proc_timer()
{
	//kprintf("proc_timer %08x\n", proc_list);
	//
	if (schedlock)
	{
		//kprintf("2");
		return;
	}

	if (proc_list == NULL)
	{
		//kprintf("Proc_list == null\n");
		return;
	}

	lock_spinlock(&schedlock);
	proc_current[cpu_id()] = proc_first_idle();
	unlock_spinlock(&schedlock);
}

int proc_ended(struct process* proc)
{
	int r = ubasic_finished(proc->code);
	return r;
}

