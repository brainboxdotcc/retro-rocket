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
} GateDescriptor64;

#endif
