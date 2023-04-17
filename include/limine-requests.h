/**
 * @file limine-requests.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#pragma once 
#include <limine.h>

volatile struct limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .stack_size = (1024 * 1024 * 32),
};

volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
};

volatile struct limine_kernel_address_request address_request = {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST,
	.revision = 0,
};

