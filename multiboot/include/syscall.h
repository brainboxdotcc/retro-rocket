#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "kernel.h"

/* NOTE: Not used at present, may not be used for forseeable future */

/* Terminal Control */
#define SYS_PUTCH	0x100
#define SYS_PUTS	0x101
#define SYS_CLEAR	0x102
#define SYS_SETBGCLR	0x103
#define SYS_SETFGCLR	0x104

/* FS control */
#define SYS_FOPEN	0x200
#define SYS_FCLOSE	0x203
#define SYS_FREAD	0x204

/* Memory manager control */
#define SYS_MALLOC	0x305
#define SYS_FREE	0x306

/* Argument list for syscalls with more than 3 arguments */
typedef struct {
	u32int cnt;
	u32int* vect;
} argv_t;

void init_syscall();

#endif

