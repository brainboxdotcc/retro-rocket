#ifndef __STRING_H__
#define __STRING_H__

#include "kernel.h"

unsigned int strlen(const char* str);

unsigned char tolower(unsigned char input);

int strcmp(const char* s1, const char* s2);

u32int hextoint(const char* n1);

#endif
