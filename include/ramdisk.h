/**
 * @file ramdisk.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#pragma once

#include "kernel.h"

const char* init_ramdisk(size_t blocks, size_t blocksize);

const char* init_ramdisk_from_storage(const char* storage);