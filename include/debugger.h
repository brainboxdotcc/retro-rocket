/**
 * @file debugger.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#define CANONICAL_ADDRESS(x) (((((intptr_t)(x)) << 16) >> 16) == (intptr_t)(x))

typedef struct symbol
{
	char* name;
	uint64_t address;
	uint8_t type;
	struct symbol* next;
} symbol_t;

typedef struct stack_frame {
	struct stack_frame *next;
	void *addr;
} stack_frame_t;

#define DEBUG_DST_PORT 2000

#define MAX_DECODE  128
#define DATA_ROT  7
#define DATA_XOR  0x5A
#define GDB_TRIGGER 0xC536CE6E

/**
 * @brief print the contents of the specified memory address in hexadecimal
 * format along with the corresponding ASCII characters. The function achieves
 * this by iterating through the memory in chunks of 16 bytes and printing
 * each chunk on a separate line.
 * 
 * @param address a pointer to some memory address
 * @param length represents the number of bytes of memory that should be dumped.
 */
void dump_hex(const void* address, uint64_t length);

// Initialise debugger, read symbols from boot device. These are used for backtraces.
void init_debug();

void backtrace();

symbol_t* get_sym_table();

bool set_debug_signal(bool status);
bool get_debug_signal();
void gdb_decode(char* out, const uint8_t* in, size_t len);
void gdb_emit();
uint32_t gdb_trace(const char* str);

const char* findsymbol(uint64_t address, uint64_t* offset);

uint64_t findsymbol_addr(const char *name);

bool running_under_qemu(void);