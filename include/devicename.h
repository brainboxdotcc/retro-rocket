#pragma once

#include "kernel.h"

typedef struct devname_prefix_t {
	char prefix[16];
	uint8_t increment;
} devname_prefix_t;

bool make_unique_device_name(const char* prefix, char* buffer);

void init_devicenames();

