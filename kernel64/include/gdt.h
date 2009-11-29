#ifndef __GDT_H__
#define __GDT_H__

typedef struct
{
	u32 lolimit:16;		/* bits 15 to 0 of segment limit */
	u32 lobase:16;		/* bits 15 to 0 of segment base */
	u32 midbase:8;		/* bits 23 to 16 of segment base */
	u32 type:5;		/* segment type and S bit */
	u32 prio:2;		/* priority level */
	u32 present:1;		/* present? */
	u32 hilimit:4;		/* bits 16 to 19 of segment limit */
	u32 available:1;	/* available for OS use */
	u32 longmode:1;		/* long mode (code segments) */
	u32 def32:1;		/* 32 vs 16 bits (always 0) */
	u32 granularity:1;	/* limit granularity */
	u32 hibase:8;		/* bits 31 to 24 of segment base */
} GDTEntry64;

typedef struct
{
	u16 limit;	/* limit */
	u64 base;	/* base address  */
} GDTIDT64;

typedef struct
{
	u32 looffset:16;	/* bits 15 to 0 of segment code offset */
	u32 selector:16;	/* selector */
	u32 ist:3;		/* Index into IST table */
	u32 reserved:5;		/* unused */
	u32 type:5;		/* segment type and S bit */
	u32 prio:2;		/* priority level */
	u32 present:1;		/* present? */
	u32 hioffset:16;	/* bits 31:16 of segment code offset */
	u32 hi64offset:32;	/* bits 63:32 of segment code offset */
	u32 reserved2:8;	/* unused */
	u32 zero:5;
	u32 reserved3:19;	/* reserved */
} Gate64;

/* IDT Entries for exceptions */
#define	IDT_EX_DIV		0	/* Divide Error */
#define	IDT_EX_DEBUG		1	/* Debug */
#define	IDT_EX_NMI		2	/* Non-maskable Interrupt */
#define	IDT_EX_BREAKPOINT	3	/* Breakpoint */
#define	IDT_EX_OVERFLOW		4	/* Overflow */
#define	IDT_EX_BOUNDRANGE	5	/* Bound Range Exceeded */
#define	IDT_EX_INVALIDOPCODE	6	/* Undefined/Invalid Opcode */
#define	IDT_EX_NOCOPROCESSOR	7	/* No Math Coprocessor */
#define	IDT_EX_DOUBLEFAULT	8	/* Double Fault */
#define	IDT_EX_COPROCESSOROVER	9	/* Coprocessor Segment Overrun */
#define	IDT_EX_INVALIDTSS	10	/* Invalid TSS */
#define	IDT_EX_NOTPRESENT	11	/* Segment Not Present */
#define	IDT_EX_STACKFAULT	12	/* Stack Segment Fault */
#define	IDT_EX_GENERALPROT	13	/* General Protection Fault */
#define	IDT_EX_PAGEFAULT	14	/* Page Fault */
#define	IDT_EX_FPUERROR		16	/* FPU Floating-Point Error */
#define	IDT_EX_ALIGNMENTCHECK	17	/* Alignment Check */
#define	IDT_EX_MACHINECHECK	18	/* Machine Check */
#define	IDT_EX_SIMDFPU		19	/* SIMD Floating-Point Exception */

#define	IDTSZ		256	/* Number of entries in interrupt descriptor table */

#define	BYTES   0
#define PAGES	1

#define OP32	1	

#define LONG	1

#define GDT_NULL	0	/* Null selector */
#define GDT_DATA64	1	/* Data selector */
#define GDT_CODE64 	2	/* 64 bit code */

#define	IDXTOSEL(s)	((s) << 3)		/* index to selector */
#define	SEL_GDT(s, r)	(IDXTOSEL(s) | r)	/* global sel */

#define	SDT_SYSIGT	14	/* system interrupt gate */

#endif
