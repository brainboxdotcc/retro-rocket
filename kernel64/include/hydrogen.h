#ifndef __HYDROGEN_H__
#define __HYDROGEN_H__

#define MEMORY_KERNEL_VADDR             0xFFFFFF0000000000
#define MEMORY_KERNEK_STACK_VADDR       0xFFFFFF1000000000
#define MEMORY_FRAME_MAP_VADDR          0xFFFFFF2000000000
#define MEMORY_HEAP_VADDR               0xFFFFFF3000000000
#define MEMORY_HYDROGEN_VADDR           0xFFFFFF4000000000
#define MEMORY_LAPIC_VADDR              0xFFFFFF5000000000
#define MEMORY_TMP_AREA_VADDR           0xFFFFFF6000000000

//------------------------------------------------------------------------------
// Memory Layout - Info
//------------------------------------------------------------------------------

#define HYDROGEN_MEMORY		0x104000
#define HYDROGEN_INFO		0x0
#define HYDROGEN_INFO_PROC	0x01000
#define HYDROGEN_INFO_MODS	0x02000
#define HYDROGEN_INFO_MMAP	0x03000
#define HYDROGEN_INFO_IOAPIC	0x04000
#define HYDROGEN_INFO_STRINGS	0x05000

//------------------------------------------------------------------------------
// Memory Layout - System
//------------------------------------------------------------------------------

#define HYDROGEN_SYS_IDT64	0x06000
#define HYDROGEN_SYS_GDT64	0x07000

//------------------------------------------------------------------------------
// Memory Layout - Paging
//------------------------------------------------------------------------------

#define HYDROGEN_PAGE_PML4		0x08000
#define HYDROGEN_PAGE_PDP_IDN		0x09000
#define HYDROGEN_PAGE_PD_IDN		0x0A000
#define HYDROGEN_PAGE_PDP_KERNEL	0x4A000
#define HYDROGEN_PAGE_PD_KERNEL		0x4B000
#define HYDROGEN_PAGE_PT_KERNEL		0x4C000

//------------------------------------------------------------------------------
// Memory Layout - Stack
//------------------------------------------------------------------------------

#define HYDROGEN_STACK			0x4D000
#define HYDROGEN_END			0x6E000

//------------------------------------------------------------------------------
// Flags
//------------------------------------------------------------------------------

// Flags for HydrogenInfo
#define HYDROGEN_FLAG_PIC		(1 << 0) // Presence of 8259 PIC

// Flags for HydrogenInfoProcs
#define HYDROGEN_PROC_FLAG_PRESENT	(1 << 0)
#define HYDROGEN_PROC_FLAG_BSP		(1 << 1)
#define HYDROGEN_PROC_FLAG_READY	(1 << 2)

//------------------------------------------------------------------------------
// Structures
//------------------------------------------------------------------------------

/**
 * The main Hydrogen info table.
 */
typedef struct HydrogenInfotable
{
	u64 free_mem_begin;	// Beginning of free memory
	u64 command_line;	  // Command line for kernel
	u64 lapic_paddr;	   // Physical address of LAPIC MMIO.

	u8 flags;			  // Flags
	u8 proc_count;		 // Processor count
	u8 mod_count;		  // Module count
	u8 mmap_count;		 // Memory map entry count
	u8 ioapic_count;	   // IO APIC count

	u32 irq_to_gsi[16];	// IRQ to GSI map
	u8 irq_flags[16];	  // IRQ flags

} __attribute__((packed)) HydrogenInfo;

/**
 * A processor list entry.
 *
 * The processor list can be found at offset HYDROGEN_INFO_PROC.
 */
typedef struct HydrogenInfoproc
{

	u8 acpi_id;			// The processor's ACPI id
	u8 apic_id;			// The processor's APIC id
	u16 flags;			 // Flags
	u32 lapic_freq;		// LAPIC timer ticks per second.

} __attribute__((packed)) HydrogenInfoProcs;

/**
 * An IO APIC list entry.
 *
 * The IO APIC list can be found at offset HYDROGEN_INFO_IOAPIC.
 */
typedef struct HydrogenInfoioapic
{
	u8 id;				 // The IO APIC's id.
	u64 paddr;			 // The physical address of the MMIO region.
	u32 gsi_base;		  // The GSI base.
	u8 gsi_count;		  // The interrupt count.

} __attribute__((packed)) HydrogenInfoIOAPIC;

/**
 * A module list entry.
 *
 * The module list can be found at offset HYDROGEN_INFO_MODS.
 */
typedef struct HydrogenInfomod
{
	u64 begin;			 // The physical address of the module.
	u64 length;			// The length of the module.
	u64 cmdline;		   // Offset of cmdline string in string table.

} __attribute__((packed)) HydrogenInfoMods;

/**
 * A memory map entry.
 *
 * The memory map can be found at offset HYDROGEN_INFO_MMAP.
 */
typedef struct HydrogenInfommap
{

	u64 begin;			 // The physical address of the memory segment.
	u64 length;			// The length of the segment.
	u8 available;		  // 1 if the segment is available, 0 otherwise.

} __attribute__((packed)) HydrogenInfoMemory;

//------------------------------------------------------------------------------
// Table Pointers
//------------------------------------------------------------------------------

extern HydrogenInfo *hydrogen_info;
extern HydrogenInfoMemory *hydrogen_mmap;
extern HydrogenInfoMods *hydrogen_mods;
extern HydrogenInfoProcs *hydrogen_proc;
extern HydrogenInfoIOAPIC *hydrogen_ioapic;
extern const s8 *HydrogenStrings;

#endif
