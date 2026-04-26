#include <kernel.h>

static void proc_update_cpu_usage(void);
static void proc_destroy_owned(process_t* proc);
static void proc_reap_killed(void);
static void proc_attach_pending(void);
static bool proc_queue_start(process_t* proc);
static uint8_t proc_choose_cpu(void);

static uint32_t proc_cpu_count = 1;

typedef struct proc_start_request {
	process_t* proc;
	struct proc_start_request* next;
} proc_start_request_t;

volatile struct limine_kernel_file_request rr_kfile_req = {
	.id = LIMINE_KERNEL_FILE_REQUEST,
	.revision = 0
};

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
idle_timer_t* task_idles[MAX_CPUS] = { NULL }, *timer_idles[MAX_CPUS] = { NULL };

proc_start_request_t* proc_start_queue[MAX_CPUS] = { NULL };
spinlock_t proc_start_lock[MAX_CPUS] = { 0 };

extern simple_cv_t boot_condition;

uint64_t basic_lines = 0;

bool booted_from_cd(void) {
	if (rr_kfile_req.response) {
		struct limine_kernel_file_response *kernel_info = rr_kfile_req.response;
		return (kernel_info->kernel_file->media_type == LIMINE_MEDIA_TYPE_OPTICAL);
	}
	return false;
}

bool is_basic(const char* buf, size_t size) {
	GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
	GENERATE_ENUM_STRING_LENGTHS(TOKEN, token_name_lengths)

	const char* p = buf;
	size_t remaining = size;
	const size_t token_count = sizeof(token_names) / sizeof(*token_names);

	while (remaining) {
		uint8_t c = *p++;
		if (c < 32 && c != '\t' && c != '\r' && c != '\n' && c != 27) {
			return false;
		}
		remaining--;
	}

	for (size_t i = 0; i < token_count; i++) {
		const char* kw = token_names[i];
		size_t kw_len = token_name_lengths[i];

		if (kw_len == 0 || kw_len > size) {
			continue;
		}

		for (size_t off = 0; off <= size - kw_len; off++) {
			if (memcmp(buf + off, kw, kw_len) == 0) {
				return true;
			}
		}
	}

	return false;
}

static uint8_t proc_choose_cpu(void)
{
	static uint8_t next_cpu = 0;

	uint8_t cpu = next_cpu++;
	if (next_cpu >= proc_cpu_count) {
		next_cpu = 0;
	}

	return cpu;
}

static process_t* proc_create_common(const char* source, pid_t parent_pid, const char* csd, const char* program_name, const char* directory, size_t size)
{
	process_t* newproc;
	char* error = "Unknown error";

	if (!source || !*source) {
		kprintf("Cannot start empty program.\n");
		return NULL;
	} else if (!is_basic(source, size)) {
		kprintf("This does not look like a BASIC program.\n");
		return NULL;
	}

	newproc = kmalloc(sizeof(process_t));
	if (!newproc) {
		kprintf("Out of memory starting new process.\n");
		return NULL;
	}

	newproc->code = basic_init(source, nextid, directory, &error);
	if (!newproc->code) {
		kfree_null(&newproc);
		kprintf("Fatal error parsing program: %s\n", error);
		return NULL;
	}

	newproc->code->proc = newproc;
	newproc->waitpid = 0;
	newproc->name = strdup(program_name ? program_name : "<anonymous>");
	newproc->pid = nextid++;
	newproc->directory = strdup(directory ? directory : "<anonymous>");
	newproc->csd = strdup(csd ? csd : "/");
	newproc->size = size;
	newproc->start_time = time(NULL);
	newproc->state = PROC_RUNNING;
	newproc->ppid = parent_pid;
	newproc->cpu = proc_choose_cpu();
	newproc->check_idle = NULL;
	newproc->idle_context = NULL;
	newproc->sched_next = NULL;
	newproc->sched_prev = NULL;
	newproc->global_next = NULL;
	newproc->global_prev = NULL;
	newproc->cpu_percent = 0;
	newproc->kill_requested = false;

	if (!newproc->name || !newproc->directory || !newproc->csd) {
		basic_destroy(newproc->code);
		kfree_null(&newproc->name);
		kfree_null(&newproc->directory);
		kfree_null(&newproc->csd);
		kfree_null(&newproc);
		kprintf("Out of memory starting new process.\n");
		return NULL;
	}

	lock_spinlock(&combined_proc_lock);

	if (combined_proc_list == NULL) {
		combined_proc_list = newproc;
		newproc->global_next = NULL;
		newproc->global_prev = NULL;
	} else {
		newproc->global_next = combined_proc_list;
		newproc->global_prev = NULL;
		combined_proc_list->global_prev = newproc;
		combined_proc_list = newproc;
	}

	process_count++;
	hashmap_set(process_by_pid, &(proc_id_t){ .id = newproc->pid, .proc = newproc });

	proc_update_cpu_usage();

	unlock_spinlock(&combined_proc_lock);

	if (!proc_queue_start(newproc)) {
		lock_spinlock(&combined_proc_lock);

		if (newproc->global_prev == NULL) {
			combined_proc_list = newproc->global_next;
		} else {
			newproc->global_prev->global_next = newproc->global_next;
		}
		if (newproc->global_next != NULL) {
			newproc->global_next->global_prev = newproc->global_prev;
		}

		hashmap_delete(process_by_pid, &(proc_id_t){ .id = newproc->pid });
		process_count--;

		unlock_spinlock(&combined_proc_lock);

		basic_destroy(newproc->code);
		kfree_null(&newproc->name);
		kfree_null(&newproc->directory);
		kfree_null(&newproc->csd);
		kfree_null(&newproc);
		kprintf("Out of memory starting new process.\n");
		return NULL;
	}

	return newproc;
}

process_t* proc_load(const char* fullpath, pid_t parent_pid, const char* csd)
{
	fs_directory_entry_t* fsi;
	unsigned char* programtext;
	process_t* newproc;

	fsi = fs_get_file_info(fullpath);
	if (fsi == NULL || (fsi->flags & FS_DIRECTORY)) {
		kprintf("File does not exist.\n");
		return NULL;
	}
	if (fsi->flags & FS_DIRECTORY) {
		kprintf("Can't execute a directory.\n");
		return NULL;
	}

	programtext = kmalloc(fsi->size + 1);
	if (!programtext) {
		kprintf("Out of memory starting new process.\n");
		return NULL;
	}

	*(programtext + fsi->size) = 0;
	if (!fs_read_file(fsi, 0, fsi->size, programtext)) {
		kfree_null(&programtext);
		kprintf("Failed to read program file for new process.\n");
		return NULL;
	}

	newproc = proc_create_common((const char*)programtext, parent_pid, csd, fsi->filename, fullpath, fsi->size);
	kfree_null(&programtext);
	return newproc;
}

process_t* proc_load_anonymous(const char* source, pid_t parent_pid, const char* csd)
{
	const char* name = "<anonymous>";
	const char* directory = "<eval>";

	return proc_create_common(source, parent_pid, csd, name, directory, strlen(source));
}

process_t* proc_cur(uint8_t logical_cpu)
{
	return proc_current[logical_cpu];
}

bool check_wait_pid(process_t* proc, void* opaque) {
	if (proc_find(proc->waitpid) == NULL) {
		proc->waitpid = 0;
		return false;
	}
	return true;
}

void proc_run(process_t* proc)
{
	if (proc->check_idle && !proc->check_idle(proc, proc->idle_context)) {
		proc_set_idle(proc, NULL, NULL);
		basic_run(proc->code);
		return;
	} else if (proc->check_idle) {
		__builtin_ia32_pause();
		return;
	}
	basic_run(proc->code);
	basic_lines++;
}

process_t* proc_find(pid_t pid)
{
	process_t* proc = NULL;
	lock_spinlock(&combined_proc_lock);
	proc_id_t* id = hashmap_get(process_by_pid, &(proc_id_t){ .id = pid });
	if (id) {
		proc = id->proc;
	}
	unlock_spinlock(&combined_proc_lock);
	return proc;
}

bool proc_kill_id(pid_t id)
{
	bool result = false;

	lock_spinlock(&combined_proc_lock);
	proc_id_t* proc_id = hashmap_get(process_by_pid, &(proc_id_t){ .id = id });
	if (proc_id) {
		process_t* proc = proc_id->proc;
		process_t* cur = proc_cur(proc->cpu);
		if (cur->pid != id) {
			proc->kill_requested = true;
			apic_send_ipi(get_lapic_id_from_cpu_id(proc->cpu), APIC_KILL_IPI);
			result = true;
		}
	}
	unlock_spinlock(&combined_proc_lock);

	return result;
}

void proc_set_idle(process_t* proc, activity_callback_t callback, void* context) {
	if (callback) {
		if (proc->check_idle) {
			dprintf("WARNING: Possible bug; process '%lu' already has an idle callback\n", proc->pid);
		}
		proc->check_idle = callback;
		proc->idle_context = context;
		proc->state = PROC_SUSPENDED;
	} else {
		proc->check_idle = NULL;
		proc->idle_context = NULL;
		proc->state = PROC_RUNNING;
	}
}

void proc_wait(process_t* proc, pid_t otherpid)
{
	proc->waitpid = otherpid;
	proc_set_idle(proc, check_wait_pid, NULL);
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
		dprintf("Process %lu CSD set to: '%s'\n", proc->pid, proc->csd);
		return proc->csd;
	}

	proc->csd = krealloc((void*)proc->csd, len + csdlen + 2);
	if (len > 1) {
		strlcat((char*)proc->csd, "/", len + csdlen + 2);
	}
	strlcat((char*)proc->csd, csd, len + csdlen + 2);
	dprintf("Process %lu CSD set to: '%s'\n", proc->pid, proc->csd);
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
	proc->kill_requested = true;
	apic_send_ipi(get_lapic_id_from_cpu_id(proc->cpu), APIC_KILL_IPI);
}

static void proc_destroy_owned(process_t* proc)
{
	uint8_t cpu = proc->cpu;
	dprintf("proc_kill id %lu on cpu %d\n", proc->pid, cpu);

	if (proc->sched_prev == NULL) {
		proc_list[cpu] = proc->sched_next;
	} else {
		proc->sched_prev->sched_next = proc->sched_next;
	}
	if (proc->sched_next != NULL) {
		proc->sched_next->sched_prev = proc->sched_prev;
	}

	if (proc_current[cpu] == proc) {
		proc_current[cpu] = proc->sched_next ? proc->sched_next : proc_list[cpu];
	}

	lock_spinlock(&combined_proc_lock);

	if (proc->global_prev == NULL) {
		combined_proc_list = proc->global_next;
	} else {
		proc->global_prev->global_next = proc->global_next;
	}
	if (proc->global_next != NULL) {
		proc->global_next->global_prev = proc->global_prev;
	}

	basic_destroy(proc->code);
	kfree_null(&proc->name);
	kfree_null(&proc->directory);
	kfree_null(&proc->csd);
	hashmap_delete(process_by_pid, &(proc_id_t){ .id = proc->pid });
	kfree_null(&proc);
	process_count--;

	/* Killed the last process? */
	if (combined_proc_list == NULL) {
		setforeground(COLOUR_LIGHTRED);
		kprintf("\nSystem halted.");
		interrupts_off();
		wait_forever();
	}
	unlock_spinlock(&combined_proc_lock);
}

static void proc_reap_killed(void)
{
	uint8_t cpu = logical_cpu_id();

	for (process_t* cur = proc_list[cpu]; cur;) {
		process_t* next = cur->sched_next;

		if (cur->kill_requested) {
			proc_destroy_owned(cur);
		}

		cur = next;
	}
}

static bool proc_queue_start(process_t* proc)
{
	dprintf("proc_queue_start\n");
	proc_start_request_t* request = kmalloc(sizeof(proc_start_request_t));
	if (!request) {
		return false;
	}

	request->proc = proc;
	request->next = NULL;

	lock_spinlock(&proc_start_lock[proc->cpu]);
	request->next = proc_start_queue[proc->cpu];
	proc_start_queue[proc->cpu] = request;
	unlock_spinlock(&proc_start_lock[proc->cpu]);

	dprintf("preparing to send proc IPI to %u\n", get_lapic_id_from_cpu_id(proc->cpu));
	apic_send_ipi(get_lapic_id_from_cpu_id(proc->cpu), APIC_PROC_IPI);
	dprintf("IPI sent\n");
	return true;
}

static void proc_attach_pending(void)
{
	dprintf("proc_attach_pending\n");
	uint8_t cpu = logical_cpu_id();

	while (true) {
		lock_spinlock(&proc_start_lock[cpu]);
		proc_start_request_t* request = proc_start_queue[cpu];
		if (request) {
			proc_start_queue[cpu] = request->next;
		}
		unlock_spinlock(&proc_start_lock[cpu]);

		if (!request) {
			dprintf("done\n");
			return;
		}

		dprintf("request: %p\n", request);

		process_t* proc = request->proc;
		kfree(request);

		dprintf("step\n");

		if (proc_list[cpu] == NULL) {
			proc_list[cpu] = proc;
			proc->sched_next = NULL;
			proc->sched_prev = NULL;
			dprintf("to empty list\n");
		} else {
			proc->sched_next = proc_list[cpu];
			proc->sched_prev = NULL;
			proc_list[cpu]->sched_prev = proc;
			proc_list[cpu] = proc;
			dprintf("to non-empty list\n");
		}

		if (proc_current[cpu] == NULL) {
			dprintf("set current\n");
			proc_current[cpu] = proc_list[cpu];
		}
	}
}

int64_t proc_total()
{
	int64_t total;

	lock_spinlock(&combined_proc_lock);
	total = process_count;
	unlock_spinlock(&combined_proc_lock);

	return total;
}

pid_t proc_id(int64_t index)
{
	int64_t tot = 0;
	lock_spinlock(&combined_proc_lock);
	for (process_t* cur = combined_proc_list; cur; cur = cur->global_next) {
		if (tot == index) {
			unlock_spinlock(&combined_proc_lock);
			return cur->pid;
		}
		tot++;
	}
	unlock_spinlock(&combined_proc_lock);
	return 0;
}

void proc_run_next(uint8_t cpu)
{
	process_t* current = proc_current[cpu];
	if (current == NULL) {
		__builtin_ia32_pause();
		return;
	}

	proc_current[cpu] = current->sched_next ? current->sched_next : proc_list[cpu];

	current = proc_current[cpu];

	if (current->kill_requested) {
		return;
	}

	proc_run(current);
	if (proc_ended(current)) {
		if (current->code->claimed_flip) {
			set_video_auto_flip(true);
		}
		proc_destroy_owned(current);
	}
}

uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const proc_id_t *p = item;
	return hashmap_sip(&p->id, sizeof(p->id), seed0, seed1);
}

int process_compare(const void *a, const void *b, void *udata) {
	const proc_id_t *pa = a;
	const proc_id_t *pb = b;
	return pa->id - pb->id;
}

void wakeup_callback([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void* opaque)
{
}

void proc_callback([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void* opaque)
{
	dprintf("proc_callback on cpu %u\n", logical_cpu_id());
	proc_queue_dpc(proc_attach_pending);
}

void kill_callback([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void* opaque)
{
	proc_queue_dpc(proc_reap_killed);
}

void halt_callback([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t errorcode, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void* opaque)
{
	dprintf("Halting CPU#%d at request of HALT IPI\n", logical_cpu_id());
	register_shutdown_ap();
	interrupts_off();
	while (true) {
		__asm__ __volatile__("hlt");
	}
}

void init_process()
{
	init_spinlock(&combined_proc_lock);
	for (size_t x = 0; x < MAX_CPUS; ++x) {
		init_spinlock(&proc_lock[x]);
		init_spinlock(&proc_start_lock[x]);
	}
	process_by_pid = hashmap_new(sizeof(proc_id_t), 0, 704830503, 487304583058, process_hash, process_compare, NULL, NULL);

	if (logical_cpu_id() == 0) {
		/* Check for kernel commandline to start the installer stub */
		const char *cmd = NULL;
		if (rr_kfile_req.response != NULL && rr_kfile_req.response->kernel_file != NULL) {
			cmd = rr_kfile_req.response->kernel_file->cmdline;
		}
		if (cmd != NULL && !strcasecmp(cmd, "install")) {
			installer();
			wait_forever();
		}
	}

	if (logical_cpu_id() == 0) {
		register_interrupt_handler(APIC_WAKE_IPI, wakeup_callback, dev_zero, NULL);
		register_interrupt_handler(APIC_PROC_IPI, proc_callback, dev_zero, NULL);
		register_interrupt_handler(APIC_KILL_IPI, kill_callback, dev_zero, NULL);
		register_interrupt_handler(APIC_HALT_IPI, halt_callback, dev_zero, NULL);
	}
	dprintf("Spawning /programs/init\n");
	process_t* init = proc_load("/programs/init", 0, "/");
	if (!init) {
		preboot_fail("/programs/init missing or invalid!\n");
	}
	dprintf("Entering proc_loop\n");
	proc_loop();
}

void proc_loop()
{
	__atomic_fetch_add(&proc_cpu_count, 1, __ATOMIC_SEQ_CST);
	uint8_t cpu = logical_cpu_id();

	if (cpu == 0) {
		/* BSP signals APs to start their proc_loops too */
		simple_cv_broadcast(&boot_condition);
	} else {
		register_interrupt_handler(APIC_WAKE_IPI, wakeup_callback, dev_zero, NULL);
		register_interrupt_handler(APIC_PROC_IPI, proc_callback, dev_zero, NULL);
		register_interrupt_handler(APIC_KILL_IPI, kill_callback, dev_zero, NULL);
		register_interrupt_handler(APIC_HALT_IPI, halt_callback, dev_zero, NULL);
	}
	//proc_register_idle(proc_update_cpu_usage, IDLE_BACKGROUND, 150);
	while (true) {
		run_idles(cpu);
		if (proc_list[cpu] == NULL) {
			/* This CPU has nothing to do; prevent busy spin */
			__asm__("hlt");
			continue;
		}
		proc_run_next(cpu);
	}
}

void run_idles(uint8_t cpu)
{
	if (cpu != 0) {
		dprintf("idles on %u\n", cpu);
	}
	if (task_idles[cpu]) {
		/* Idle foreground tasks only run on BSP */
		idle_timer_t **p = &task_idles[cpu];
		while (*p) {
			idle_timer_t *i = *p;
			if (get_ticks() > i->next_tick) {
				i->func();
				if (i->frequency == 0) {
					/* One-shot timer (DPC) */
					*p = i->next;
					kfree(i);
					continue;
				}
				i->next_tick = i->frequency + get_ticks();
			}
			p = &((*p)->next);
		}
	}
}

void proc_timer()
{
	uint8_t cpu = logical_cpu_id();
	if (proc_list[cpu] == NULL || proc_current[cpu] == NULL) {
		return;
	}

	if (proc_current[cpu]->sched_next == NULL) {
		proc_current[cpu] = proc_list[cpu];
	} else {
		proc_current[cpu] = proc_current[cpu]->sched_next;
	}
}

int proc_ended(process_t* proc)
{
	return basic_finished(proc->code);
}

void proc_register_idle(proc_idle_timer_t handler, idle_type_t type, uint64_t frequency_ms)
{
	if (!handler) {
		return;
	}
	dprintf("Register idler: %d freq %lu\n", type, frequency_ms);
	idle_timer_t* newidle = kmalloc(sizeof(idle_timer_t));
	if (!newidle) {
		return;
	}
	uint8_t cpu = logical_cpu_id();
	newidle->func = handler;
	newidle->next_tick = get_ticks() + frequency_ms;
	newidle->frequency = frequency_ms;
	if (type == IDLE_FOREGROUND) {
		newidle->next = task_idles[cpu];
		task_idles[cpu] = newidle;
	} else {
		newidle->next = timer_idles[cpu];
		timer_idles[cpu] = newidle;
	}
}

void proc_queue_dpc(dpc_t handler)
{
	dprintf("Queue DPC\n");
	if (!handler) {
		return;
	}

	idle_timer_t* newidle = kmalloc(sizeof(idle_timer_t));
	if (!newidle) {
		return;
	}

	uint8_t cpu = logical_cpu_id();

	newidle->func = handler;
	newidle->next_tick = get_ticks();
	newidle->frequency = 0;
	newidle->next = task_idles[cpu];

	task_idles[cpu] = newidle;
}

static void proc_update_cpu_usage(void) {

	uint32_t runnable = 0;
	const uint8_t cpu = logical_cpu_id();

	for (process_t* cur = proc_list[cpu]; cur; cur = cur->sched_next) {
		if (cur->state == PROC_RUNNING) {
			runnable++;
		}
	}

	for (process_t* cur = proc_list[cpu]; cur; cur = cur->sched_next) {
		uint32_t sample = 0;

		if (runnable && cur->state == PROC_RUNNING) {
			sample = 100 / runnable;
		}

		cur->cpu_percent = ((cur->cpu_percent * 31) + sample) / 32;
	}
}

uint32_t proc_cpu_percent(pid_t pid)
{
	uint32_t perc = 0;
	process_t* proc = NULL;
	lock_spinlock(&combined_proc_lock);
	proc_id_t* id = hashmap_get(process_by_pid, &(proc_id_t){ .id = pid });
	if (id) {
		proc = id->proc;
	}
	if (proc) {
		perc = proc->cpu_percent;
	}
	unlock_spinlock(&combined_proc_lock);
	return perc;
}