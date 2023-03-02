#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "kernel.h"

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

/* Process manager Control */
#define SYS_FORK	0x400
#define SYS_EXEC	0x401
#define SYS_GETPID	0x402
#define SYS_FSWITCH	0x403
#define SYS_SETMTX	0x405
#define SYS_CLRMTX	0x406
#define SYS_WAITPID	0x407
#define SYS_EXIT	0x408
#define SYS_NAME_PROC	0x409

/* Argument Vector gia routines me >3 arguments */
typedef struct {
	u32int cnt;
	u32int* vect;
} argv_t;

void init_syscall();

#endif

