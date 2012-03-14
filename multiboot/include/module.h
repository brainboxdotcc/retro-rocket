#ifndef __MOD_H__
#define __MOD_H__

#include "kernel.h"
#include "debugger.h"

#define MODSTART 0x600

typedef u32int (*sym_find)(const char*);

typedef struct KernelInfoTag
{
	symbol_t* symbols;
	sym_find findsym;
} KernelInfo;

typedef int (*mod_init)(KernelInfo*);
typedef int (*mod_fini)(KernelInfo*);

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


int init_modules();
int load_module(const char* name);

#endif
