#include <kernel.h>

struct process* proc_current[256] = { NULL };	/* This can be changed by an interrupt, MAKE A COPY */
struct process* proc_list[256] = { NULL };
static struct process* proc_running[256] = { NULL };	/* This is the safe copy of proc_current */
spinlock locks[256] = { 0 };

uint32_t nextid = 1;
uint8_t nextcpu = 0;
spinlock bkl = 0;

idle_timer_t* task_idles = NULL, *timer_idles = NULL;

struct process* proc_load(const char* fullpath, struct console* cons)
{
	//dump_hex(locks, 256 * 4);
	fs_directory_entry_t* fsi = fs_get_file_info(fullpath);
	if (fsi != NULL) {
		unsigned char* programtext = (unsigned char*)kmalloc(fsi->size + 1);
		*(programtext + fsi->size) = 0;
		if (fs_read_file(fsi, 0, fsi->size, programtext)) {
			//kprintf("program len = %d size = %d cpu=%d\n", strlen(programtext), fsi->size, nextcpu);
			struct process* newproc = (struct process*)kmalloc(sizeof(struct process));
			newproc->code = ubasic_init((const char*)programtext, (console*)cons, nextid);
			newproc->waitpid = 0;
			newproc->name = strdup(fsi->filename);
			newproc->pid = nextid++;
			newproc->size = fsi->size;
			newproc->cons = cons;
			newproc->cpu = nextcpu;
			//newproc->text = programtext;
			kfree(programtext);

			interrupts_off();
			bkl = 1;

			if (proc_list[nextcpu] == NULL) {
				/* First process */
				proc_list[nextcpu] = newproc;
				newproc->next = NULL;
				newproc->prev = NULL;
			} else {
				/* Any other process */
				newproc->next = proc_list[nextcpu];
				newproc->prev = NULL;
				proc_list[nextcpu]->prev = newproc;
				proc_list[nextcpu] = newproc;
			}

			// No current proc? Make it the only proc.
			if (proc_current[nextcpu] == NULL)
				proc_current[nextcpu] = proc_list[nextcpu];

			nextcpu++;
			if (nextcpu > 0)
				nextcpu = 0;

			bkl = 0;
			interrupts_on();

			//kprintf("returning\n");

			return newproc;
		} else {
			kfree(programtext);
		}

		return NULL;
	}

	return NULL;
}

struct process* proc_cur()
{
	return proc_running[cpu_id()];
}

void proc_run(struct process* proc)
{
	proc_running[cpu_id()] = proc;
	if (proc->waitpid == 0)
		ubasic_run(proc->code);
	else if (proc_find(proc->waitpid) == NULL) {
		//kprintf("Process %d exited, resuming %d\n", proc->waitpid, proc->pid);
		proc->waitpid = 0;
		ubasic_run(proc->code);
	}
	//else
	//	kprintf("proc_find(%d) == %d\n.\n", proc->waitpid, proc_find(proc->waitpid));
}

struct process* proc_find(uint32_t pid)
{
	interrupts_off();
	bkl = 1;
	struct process* foundproc = NULL;
	int cpu = 0;
	for (; cpu < 1; cpu++) {
		struct process* cur = proc_list[cpu];
		for (; cur; cur = cur->next)
			if (cur->pid == pid)
				foundproc = cur;
	}
	bkl = 0;
	interrupts_on();

	return foundproc;
}

void proc_wait(struct process* proc, uint32_t otherpid)
{
	//kprintf("Process waiting for pid %d\n", otherpid);
	proc->waitpid = otherpid;
}

void proc_kill(struct process* proc)
{
	interrupts_off();
	bkl = 1;

	struct process* cur = proc_list[proc->cpu];

	int countprocs = 0;
	for (; cur; cur = cur->next) {
		countprocs++;
		if (cur->pid == proc->pid) {
			if (proc->next == NULL && proc->prev == NULL) {
				// the only process!
				proc_list[proc->cpu] = NULL;
			} else if (proc->prev == NULL && proc->next != NULL) {
				// first item
				proc_list[proc->cpu] = proc->next;
				proc_list[proc->cpu]->prev = NULL;
			} else if (proc->prev != NULL && proc->next == NULL) {
				// last item
				proc->prev->next = NULL;
			} else {
				// middle item
				proc->prev->next = proc->next;
				proc->next->prev = proc->prev;
			}

			break;
		}
	}

	proc_current[proc->cpu] = proc_list[proc->cpu];

	ubasic_destroy(proc->code);
	kfree(proc->name);
	//kfree(proc->text);

	/* Killed the last process? */
	int cpun = 0, proclists = 0;
	for (; cpun < 256; cpun++)
		if (proc_list[cpun] != NULL)
			proclists++;

	if (proclists == 0) {
		setforeground(current_console, COLOUR_LIGHTRED);
		kprintf("\nSystem halted.");
		wait_forever();
	}

	bkl = 0;
	interrupts_on();
}

void proc_show_list()
{
	interrupts_off();
	int cpu = 0;
	for (; cpu < 256; cpu++) {
		struct process* cur = proc_list[cpu];
		for (; cur; cur = cur->next) {
			kprintf("PID %d CPU %d name %s\n", cur->pid, cpu, cur->name);
		}
	}
	interrupts_on();
}

int64_t proc_total()
{
	interrupts_off();
	int64_t tot = 0;
	int cpu = 0;
	for (; cpu < 256; cpu++) {
		struct process* cur = proc_list[cpu];
		for (; cur; cur = cur->next) {
			tot++;
		}
	}
	return tot;
	interrupts_on();
}

const char* proc_name(int64_t index)
{
	interrupts_off();
	int64_t tot = 0;
	int cpu = 0;
	for (; cpu < 256; cpu++) {
		struct process* cur = proc_list[cpu];
		for (; cur; cur = cur->next) {
			if (tot == index) {
				interrupts_on();
				return cur->name;
			}
			tot++;
		}
	}
	interrupts_on();
	return "";
}

uint32_t proc_id(int64_t index)
{
	interrupts_off();
	int64_t tot = 0;
	int cpu = 0;
	for (; cpu < 256; cpu++) {
		struct process* cur = proc_list[cpu];
		for (; cur; cur = cur->next) {
			if (tot == index) {
				interrupts_on();
				return cur->pid;
			}
			tot++;
		}
	}
	interrupts_on();
	return 0;
}

void proc_run_next()
{
	struct process* current;
	interrupts_off();
	bkl = 1;
	current = proc_current[cpu_id()];
	bkl = 0;
	interrupts_on();

	if (current != NULL) {
		proc_run(current);
		if (proc_ended(current)) {
			proc_kill(current);
		}
	}
}

void proc_loop()
{
	while (true) {
		while (bkl);
		proc_timer();
		proc_run_next();
		idle_timer_t* i = task_idles;
		for (; i; i = i->next) {
			i->func();
		}
	}
}

void proc_timer()
{
	bkl = 1;
	if (proc_list[cpu_id()] == NULL)
		return;

	if (proc_current[cpu_id()]->next == NULL)
		proc_current[cpu_id()] = proc_list[cpu_id()];
	else
		proc_current[cpu_id()] = proc_current[cpu_id()]->next;
	bkl = 0;
	//kprintf("%08x", proc_current);
}

int proc_ended(struct process* proc)
{
	int r = ubasic_finished(proc->code);
	return r;
}

void proc_register_idle(proc_idle_timer_t handler, idle_type_t type)
{
	/* Add the new filesystem to the start of the list */
	idle_timer_t* newidle = kmalloc(sizeof(idle_timer_t));
	newidle->func = handler;
	if (type == IDLE_FOREGROUND) {	
		newidle->next = task_idles;
		task_idles = newidle;
	} else {
		newidle->next = timer_idles;
		timer_idles = newidle;
	}
}
