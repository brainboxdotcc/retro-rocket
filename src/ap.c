#include <kernel.h>
#include <stdatomic.h>

volatile struct limine_smp_request smp_request = {
	.id = LIMINE_SMP_REQUEST,
	.revision = 0
};

size_t aps_online = 0;
simple_cv_t boot_condition;

void kmain_ap(struct limine_smp_info *info)
{
	// Load the shared IDT
	__asm__ volatile("lidtq (%0)" :: "r"(&idt64));
	interrupts_on();

	kprintf("CPU: %u online; ID: %u\n", info->processor_id, info->lapic_id);
	atomic_fetch_add(&aps_online, 1);

	simple_cv_wait(&boot_condition);
	/**
	 * @todo Insert cpu-local scheduler loop here.
	 * Each AP will run its own list of executing BASIC processes. Accessing
	 * the list of other APs and the BSP will be strictly controlled via a
	 * marshalled lookup system using a spinlock, e.g. if AP 1 wants to start a new
	 * process on AP 2, it signals via this system.
	 *
	 * This will be done as follows:
	 *
	 * 1) Each AP can be instructed via a command queue to launch, query or kill a process.
	 * 2) Each AP will have its own command queue
	 * 3) Any AP can push a command onto the command queue for one or more other APs to action
	 * 4) Initially AP's will wait for a start command in their queue,
	 *    they won't run their scheduler until they receive this command. This allows them all to
	 *    gracefully wait until the first BASIC process is ready to be loaded (/programs/init).
	 * 5) A shared process list will be used for enumerating processes and checking process
	 *    state, so we dont have to peek at another scheduler instance's command queue to
	 *    see if a process ours is waiting on still lives.
	 */
	 dprintf("Got start signal on cpu #%d\n", info->processor_id);

	 proc_loop();
}
