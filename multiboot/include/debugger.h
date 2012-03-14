#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "kernel.h"
#include "interrupts.h"

#define SYM_ABSOLUTE 'A'
#define SYM_BSS 'B'
#define SYM_COMMON 'C'
#define SYM_INITIALISED 'D'
#define SYM_SMALLOBJECT 'G'
#define SYM_INDIRECT_REF 'I'
#define SYM_DEBUGGING 'N'
#define SYM_READONLY 'R'
#define SYM_UNINITIALISED 'S'
#define SYM_TEXT 'T'
#define SYM_TEXT2 't'
#define SYM_UNDEFINED 'U'
#define SYM_WEAK_OBJECT 'V'
#define SYM_WEAK_SYMBOL 'W'
#define SYM_STABS '-'
#define SYM_UNKNOWN '?'

typedef struct symbol
{
	char* name;
	u32int address;
	u8int type;
	struct symbol* next;
} symbol_t;

typedef struct stack_frame {
	struct stack_frame *next;
	void *addr;
} stack_frame_t;

// Create a hex dump of a region of ram, displayed in BBC Miro/Archimedes *DUMP style,
// dumped to current_console.
void DumpHex(unsigned char* address, u32int length);

// Initialise debugger, read symbols from boot device. These are used for backtraces.
void init_debug();

void backtrace(registers_t* regs);

symbol_t* get_sym_table();

#endif
