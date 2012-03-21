#include "../include/kernel.h"
#include "../include/hydrogen.h"

HydrogenInfo *hydrogen_info = (HydrogenInfo *) (MEMORY_HYDROGEN_VADDR + HYDROGEN_INFO);
HydrogenInfoMemory *hydrogen_mmap = (HydrogenInfoMemory *) (MEMORY_HYDROGEN_VADDR + HYDROGEN_INFO_MMAP);
HydrogenInfoMods *hydrogen_mods = (HydrogenInfoMods *) (MEMORY_HYDROGEN_VADDR + HYDROGEN_INFO_MODS);
HydrogenInfoProcs *hydrogen_proc = (HydrogenInfoProcs *) (MEMORY_HYDROGEN_VADDR + HYDROGEN_INFO_PROC);
HydrogenInfoIOAPIC *hydrogen_ioapic = (HydrogenInfoIOAPIC *) (MEMORY_HYDROGEN_VADDR + HYDROGEN_INFO_IOAPIC);
const s8 *HydrogenStrings = (const s8 *) (MEMORY_HYDROGEN_VADDR + HYDROGEN_INFO_STRINGS);

