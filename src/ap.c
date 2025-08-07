/**
 * @file ap.c
 * @brief Bring up and enter the process loop for an AP
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */

#include <kernel.h>

volatile struct limine_smp_request smp_request = {
	.id = LIMINE_SMP_REQUEST,
	.revision = 0
};

size_t aps_online = 0;
simple_cv_t boot_condition;

void wait_for_interpreter_start(struct limine_smp_info *info) {
	kprintf("%u ", info->processor_id);
	atomic_fetch_add(&aps_online, 1);
	simple_cv_wait(&boot_condition);
}

void kmain_ap(struct limine_smp_info *info) {
	load_ap_shared_idt();
	wait_for_interpreter_start(info);
	apic_setup_ap();
	proc_loop();
}
