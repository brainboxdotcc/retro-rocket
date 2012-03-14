#ifndef __MOD_H__
#define __MOD_H__

#include "kernel.h"

#define MODSTART 0x600

typedef struct KernelInfoTag
{
	int dummy;
} KernelInfo;

typedef int (*mod_init)(int, KernelInfo*);
typedef int (*mod_fini)(int, KernelInfo*);

typedef struct ModuleHeaderTag
{
	u32int signature;
	mod_init modinit;
	mod_fini modfini;
} __attribute__((packed)) ModuleHeader;

typedef struct ModuleTag
{
	u8int* progbits;
	char* name;
	u32int size;
	ModuleHeader* header;
	struct ModuleTag* next;
} Module;


int load_module(const char* name);

#endif
