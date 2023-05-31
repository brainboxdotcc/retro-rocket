/**
 * @file memcpy.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#pragma once

#include "kernel.h"

void* memcpy(void* dest, const void* src, uint64_t len);
void* memmove(void* dest, const void* src, uint64_t n);
int memcmp(const void* s1, const void* s2, uint64_t n);
void memset(void* dest, char val, uint64_t len);

size_t memrev(char* buf, size_t n);
