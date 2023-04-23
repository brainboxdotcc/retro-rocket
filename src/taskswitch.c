#include <kernel.h>

/**
 * @brief The currently running process
 */
process_t* proc_current = NULL;
/**
 * @brief Doubly linked list of processes.
 * We store the doubly linked list for fast iteration from one process
 * to the next for the round robin scheduler of processes.
 */
process_t* proc_list = NULL;
/**
 * @brief A hash map of processes by PID.
 * We use this when we want to find a process by PID (which we do quite often
 * whenever a process waits on another process). This makes it faster to look
 * up a process by PID instead of having to iteate the doubly linked list we
 * use for scheduling.
 */
struct hashmap* process_by_pid = NULL;
/**
 * @brief Next process ID number we will give out for the next process.
 * This increments by one, but will not reuse "holes", so it behaves in
 * a generally posix-like manner.
 */
uint32_t nextid = 1;
/**
 * @brief A counter of the number of active processes, we use this to
 * find out how many processes are running and iterate it or decrement it
 * as processes are killed or started so that to find out the total count
 * we do not need to iterate either the hash map or the doubly linked list.
 */
uint32_t process_count = 0;

/**
 * @brief Lists of "idle tasks", idle tasks are simple kernel functions
 * that execute once per cycle around the process loop, used for things that
 * we do not want to put into an interrupt, but still need to run often.
 * timer idles on the other hand are inserted into the LAPIC timer,
 * and need to be written to be friendly within an interrupt context.
 */
idle_timer_t* task_idles = NULL, *timer_idles = NULL;

process_t* proc_load(const char* fullpath, struct console* cons, pid_t parent_pid)
{
	fs_directory_entry_t* fsi = fs_get_file_info(fullpath);
	if (fsi != NULL && !(fsi->flags & FS_DIRECTORY)) {
		unsigned char* programtext = kmalloc(fsi->size + 1);
		*(programtext + fsi->size) = 0;
		if (fs_read_file(fsi, 0, fsi->size, programtext)) {
			process_t* newproc = kmalloc(sizeof(process_t));
			char* error = "Unknown error";
			newproc->code = ubasic_init((const char*)programtext, (console*)cons, nextid, fullpath, &error);
			if (!newproc->code) {
				kfree(newproc);
				kfree(programtext);
				kprintf("Fatal error parsing program: %s\n", error);
				return NULL;
			}
			newproc->waitpid = 0;
			newproc->name = strdup(fsi->filename);
			newproc->pid = nextid++;
			newproc->directory = strdup(fullpath);
			newproc->size = fsi->size;
			newproc->start_time = time(NULL);
			newproc->state = PROC_RUNNING;
			newproc->ppid = parent_pid;
			newproc->cons = cons;
			newproc->cpu = 0;
			kfree(programtext);

			if (proc_list == NULL) {
				/* First process */
				proc_list = newproc;
				newproc->next = NULL;
				newproc->prev = NULL;
			} else {
				/* Any other process */
				newproc->next = proc_list;
				newproc->prev = NULL;
				proc_list->prev = newproc;
				proc_list = newproc;
			}

			// No current proc? Make it the only proc.
			if (proc_current == NULL) {
				proc_current = proc_list;
			}

			process_count++;
			hashmap_set(process_by_pid, &(proc_id_t){ .id = newproc->pid, .proc = newproc });

			return newproc;
		} else {
			kfree(programtext);
		}

		return NULL;
	}

	return NULL;
}

process_t* proc_cur()
{
	return proc_current;
}

void proc_run(process_t* proc)
{
	if (proc->waitpid == 0) {
		ubasic_run(proc->code);
	} else if (proc_find(proc->waitpid) == NULL) {
		proc->waitpid = 0;
		ubasic_run(proc->code);
	}
}

process_t* proc_find(pid_t pid)
{
	proc_id_t* id = hashmap_get(process_by_pid, &(proc_id_t){ .id = pid });
	return id ? id->proc : NULL;
}

bool proc_kill_id(pid_t id)
{
	process_t* cur = proc_cur();
	process_t* proc = proc_find(id);
	if (cur->pid == id) {
		return false;
	}
	if (proc) {
		proc_kill(proc);
		return true;
	}
	return false;
}

void proc_wait(process_t* proc, pid_t otherpid)
{
	if (!proc_find(otherpid)) {
		/* Process would wait forever */
		return;
	}
	proc->waitpid = otherpid;
}

void proc_kill(process_t* proc)
{
	for (process_t* cur = proc_list; cur; cur = cur->next) {
		if (cur->pid == proc->pid) {
			if (proc->next == NULL && proc->prev == NULL) {
				// the only process!
				proc_list = NULL;
			} else if (proc->prev == NULL && proc->next != NULL) {
				// first item
				proc_list = proc->next;
				proc_list->prev = NULL;
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

	proc_current = proc_list;

	ubasic_destroy(proc->code);
	kfree(proc->name);
	kfree(proc->directory);
	hashmap_delete(process_by_pid, &(proc_id_t){ .id = proc->pid });
	kfree(proc);
	process_count--;

	/* Killed the last process? */
	if (proc_list == NULL) {
		setforeground(current_console, COLOUR_LIGHTRED);
		kprintf("\nSystem halted.");
		interrupts_off();
		wait_forever();
	}
}

void proc_show_list()
{
	process_t* cur = proc_list;
	for (; cur; cur = cur->next) {
		kprintf("PID %d CPU %d name %s\n", cur->pid, cur->cpu, cur->name);
	}
}

int64_t proc_total()
{
	return process_count;
}

pid_t proc_id(int64_t index)
{
	int64_t tot = 0;
	for (process_t* cur = proc_list; cur; cur = cur->next) {
		if (tot == index) {
			return cur->pid;
		}
		tot++;
	}
	return 0;
}

void proc_run_next()
{
	process_t* current;
	current = proc_current;

	if (current != NULL) {
		proc_run(current);
		if (proc_ended(current)) {
			proc_kill(current);
		}
	}
}

uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const process_t *p = item;
	return p->pid * seed0 ^ seed1;
}

int process_compare(const void *a, const void *b, void *udata) {
	const process_t *pa = a;
	const process_t *pb = b;
	return pa->pid == pb->pid ? 0 : (pa->pid < pb->pid ? -1 : 1);
}
void init_process()
{
	process_by_pid = hashmap_new(sizeof(proc_id_t), 0, 704830503, 487304583058, process_hash, process_compare, NULL, NULL);
	process_t* init = proc_load("/programs/init", (struct console*)current_console, 0);
	if (!init) {
		preboot_fail("/programs/init missing or invalid!\n");
	}
	proc_loop();
}

void proc_loop()
{
	while (true) {
		proc_timer();
		proc_run_next();
		for (idle_timer_t* i = task_idles; i; i = i->next) {
			i->func();
		}
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
}

int proc_ended(process_t* proc)
{
	return ubasic_finished(proc->code);
}

void proc_register_idle(proc_idle_timer_t handler, idle_type_t type)
{
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
