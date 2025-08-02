#include <kernel.h>

/**
 * @brief The currently running process on each CPU
 */
process_t* proc_current[MAX_CPUS] = { NULL };
/**
 * @brief Doubly linked list of processes running on each CPU.
 * We store the doubly linked list for fast iteration from one process
 * to the next for the round robin scheduler of processes.
 */
process_t* proc_list[MAX_CPUS] = { NULL };
/**
 * @brief Combined process list for all CPUs
 */
process_t* combined_proc_list = NULL;
/**
 * @brief spinlock to guard the combined list
 */
spinlock_t combined_proc_lock = 0;
/**
 * @brief Spinlocks for each CPU to protect its lists
 */
spinlock_t proc_lock[MAX_CPUS] = { 0 };
/**
 * @brief A hash map of processes by PID on all CPUs.
 * We use this when we want to find a process by PID (which we do quite often
 * whenever a process waits on another process). This makes it faster to look
 * up a process by PID instead of having to iteate the doubly linked list we
 * use for scheduling.
 */
struct hashmap* process_by_pid;
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

extern simple_cv_t boot_condition;

process_t* proc_load(const char* fullpath, struct console* cons, pid_t parent_pid, const char* csd)
{
	fs_directory_entry_t* fsi = fs_get_file_info(fullpath);
	if (fsi == NULL || (fsi->flags & FS_DIRECTORY)) {
		kprintf("File does not exist or is a directory\n");
		return NULL;
	}
	unsigned char* programtext = kmalloc(fsi->size + 1);
	if (!programtext) {
		kprintf("Out of memory starting new process\n");
		return NULL;
	}
	*(programtext + fsi->size) = 0;
	if (!fs_read_file(fsi, 0, fsi->size, programtext)) {
		kfree_null(&programtext);
		kprintf("Failed to read program file for new process\n");
		return NULL;
	}
	process_t* newproc = kmalloc(sizeof(process_t));
	if (!newproc) {
		kfree_null(&programtext);
		kprintf("Out of memory starting new process\n");
		return NULL;
	}
	char* error = "Unknown error";
	newproc->code = basic_init((const char*)programtext, (console*)cons, nextid, fullpath, &error);
	if (!newproc->code) {
		kfree_null(&newproc);
		kfree_null(&programtext);
		kprintf("Fatal error parsing program: %s\n", error);
		return NULL;
	}
	newproc->waitpid = 0;
	newproc->name = strdup(fsi->filename);
	newproc->pid = nextid++;
	newproc->directory = strdup(fullpath);
	newproc->csd = strdup(csd);
	newproc->size = fsi->size;
	newproc->start_time = time(NULL);
	newproc->state = PROC_RUNNING;
	newproc->ppid = parent_pid;
	newproc->cons = cons;
	newproc->cpu = logical_cpu_id();
	kfree_null(&programtext);

	lock_spinlock(&combined_proc_lock);
	lock_spinlock(&proc_lock[newproc->cpu]);
	if (proc_list[newproc->cpu] == NULL) {
		/* First process */
		proc_list[newproc->cpu] = newproc;
		newproc->next = NULL;
		newproc->prev = NULL;
	} else {
		/* Any other process */
		newproc->next = proc_list[newproc->cpu];
		newproc->prev = NULL;
		proc_list[newproc->cpu]->prev = newproc;
		proc_list[newproc->cpu] = newproc;
	}
	if (combined_proc_list == NULL) {
		/* First process */
		combined_proc_list = newproc;
		newproc->next = NULL;
		newproc->prev = NULL;
	} else {
		/* Any other process */
		newproc->next = combined_proc_list;
		newproc->prev = NULL;
		combined_proc_list->prev = newproc;
		combined_proc_list = newproc;
	}
	// No current proc? Make it the only proc.
	if (proc_current[newproc->cpu] == NULL) {
		proc_current[newproc->cpu] = proc_list[newproc->cpu];
	}

	process_count++;
	hashmap_set(process_by_pid, &(proc_id_t){ .id = newproc->pid, .proc = newproc });
	unlock_spinlock(&combined_proc_lock);
	unlock_spinlock(&proc_lock[newproc->cpu]);

	return newproc;
}

process_t* proc_cur(uint8_t logical_cpu)
{
	lock_spinlock(&proc_lock[logical_cpu]);
	process_t* cur = proc_current[logical_cpu];
	unlock_spinlock(&proc_lock[logical_cpu]);
	return cur;
}

void proc_run(process_t* proc)
{
	if (proc->waitpid == 0) {
		basic_run(proc->code);
	} else if (proc_find(proc->waitpid) == NULL) {
		proc->waitpid = 0;
		basic_run(proc->code);
	}
}

process_t* proc_find(pid_t pid)
{
	lock_spinlock(&combined_proc_lock);
	proc_id_t* id = hashmap_get(process_by_pid, &(proc_id_t){ .id = pid });
	unlock_spinlock(&combined_proc_lock);
	return id ? id->proc : NULL;
}

bool proc_kill_id(pid_t id)
{
	process_t* proc = proc_find(id);
	if (!proc) {
		return false;
	}
	process_t* cur = proc_cur(proc->cpu);
	if (cur->pid == id) {
		return false;
	}
	proc_kill(proc);
	return true;
}

void proc_wait(process_t* proc, pid_t otherpid)
{
	if (!proc_find(otherpid)) {
		/* Process would wait forever */
		return;
	}
	proc->waitpid = otherpid;
}

const char* proc_set_csd(process_t* proc, const char* csd)
{
	if (!csd || !*csd || !proc) {
		return NULL;
	}

	size_t len = strlen(proc->csd), csdlen = strlen(csd);

	if (*csd == '/') {
		kfree_null(&proc->csd);
		proc->csd = strdup(csd);
		dprintf("Process %d CSD set to: '%s'\n", proc->pid, proc->csd);
		return proc->csd;
	}

	proc->csd = krealloc((void*)proc->csd, len + csdlen + 2);
	if (len > 1) {
		strlcat((char*)proc->csd, "/", len + csdlen + 2);
	}
	strlcat((char*)proc->csd, csd, len + csdlen + 2);
	dprintf("Process %d CSD set to: '%s'\n", proc->pid, proc->csd);
	return proc->csd;
}

const char* proc_get_csd(process_t* proc)
{
	if (!proc) {
		return "";
	}
	return proc->csd;
}

void proc_kill(process_t* proc)
{
	uint8_t cpu = proc->cpu;
	dprintf("proc_kill id %u on cpu %d\n", proc->pid, cpu);
	lock_spinlock(&proc_lock[cpu]);
	lock_spinlock(&combined_proc_lock);
	for (process_t* cur = proc_list[cpu]; cur; cur = cur->next) {
		if (cur->pid == proc->pid) {
			if (proc->next == NULL && proc->prev == NULL) {
				// the only process!
				proc_list[cpu] = NULL;
			} else if (proc->prev == NULL && proc->next != NULL) {
				// first item
				proc_list[cpu] = proc->next;
				proc_list[cpu]->prev = NULL;
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
	for (process_t* cur = combined_proc_list; cur; cur = cur->next) {
		if (cur->pid == proc->pid) {
			if (proc->next == NULL && proc->prev == NULL) {
				// the only process!
				combined_proc_list = NULL;
			} else if (proc->prev == NULL && proc->next != NULL) {
				// first item
				combined_proc_list = proc->next;
				combined_proc_list->prev = NULL;
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

	proc_current[cpu] = proc_list[cpu];

	basic_destroy(proc->code);
	kfree_null(&proc->name);
	kfree_null(&proc->directory);
	kfree_null(&proc->csd);
	hashmap_delete(process_by_pid, &(proc_id_t){ .id = proc->pid });
	kfree_null(&proc);
	process_count--;

	/* Killed the last process? */
	if (combined_proc_list == NULL) {
		setforeground(current_console, COLOUR_LIGHTRED);
		kprintf("\nSystem halted.");
		interrupts_off();
		wait_forever();
	}
	unlock_spinlock(&proc_lock[cpu]);
	unlock_spinlock(&combined_proc_lock);
}

int64_t proc_total()
{
	return process_count;
}

pid_t proc_id(int64_t index)
{
	int64_t tot = 0;
	lock_spinlock(&combined_proc_lock);
	for (process_t* cur = combined_proc_list; cur; cur = cur->next) {
		if (tot == index) {
			unlock_spinlock(&combined_proc_lock);
			return cur->pid;
		}
		tot++;
	}
	unlock_spinlock(&combined_proc_lock);
	return 0;
}

void proc_run_next()
{
	process_t* current = proc_current[logical_cpu_id()];
	if (current == NULL) {
		_mm_pause();
		return;
	}
	proc_run(current);
	if (proc_ended(current)) {
		if (current->code->claimed_flip) {
			set_video_auto_flip(true);
		}
		proc_kill(current);
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
	init_spinlock(&combined_proc_lock);
	for (size_t x = 0; x < MAX_CPUS; ++x) {
		init_spinlock(&proc_lock[x]);
	}
	process_by_pid = hashmap_new(sizeof(proc_id_t), 0, 704830503, 487304583058, process_hash, process_compare, NULL, NULL);
	process_t* init = proc_load("/programs/init", (struct console*)current_console, 0, "/");
	if (!init) {
		preboot_fail("/programs/init missing or invalid!\n");
	}
	proc_loop();
}

void proc_loop()
{
	uint8_t cpu = logical_cpu_id();
	if (cpu == 0) {
		/* BSP signals APs to start their proc_loops too */
		simple_cv_broadcast(&boot_condition);
	}
	while (true) {
		proc_timer();
		proc_run_next();
		if (cpu == 0) {
			/* Idle foreground tasks only run on BSP */
			for (idle_timer_t *i = task_idles; i; i = i->next) {
				i->func();
			}
		}
	}
}

void proc_timer()
{
	uint8_t cpu = logical_cpu_id();
	lock_spinlock(&proc_lock[cpu]);
	if (proc_list[cpu] == NULL || proc_current[cpu] == NULL) {
		unlock_spinlock(&proc_lock[cpu]);
		return;
	}

	if (proc_current[cpu]->next == NULL) {
		proc_current[cpu] = proc_list[cpu];
	} else {
		proc_current[cpu] = proc_current[cpu]->next;
	}

	unlock_spinlock(&proc_lock[cpu]);
}

int proc_ended(process_t* proc)
{
	return basic_finished(proc->code);
}

void proc_register_idle(proc_idle_timer_t handler, idle_type_t type)
{
	dprintf("Register idler: %d\n", type);
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
