#include <kernel.h>
#include <hydrogen.h>

HydrogenInfo *hydrogen_info = (HydrogenInfo *) (HYDROGEN_MEMORY + HYDROGEN_INFO);
HydrogenInfoMemory *hydrogen_mmap = (HydrogenInfoMemory *) (HYDROGEN_MEMORY + HYDROGEN_INFO_MMAP);
HydrogenInfoMods *hydrogen_mods = (HydrogenInfoMods *) (HYDROGEN_MEMORY + HYDROGEN_INFO_MODS);
HydrogenInfoProcs *hydrogen_proc = (HydrogenInfoProcs *) (HYDROGEN_MEMORY + HYDROGEN_INFO_PROC);
HydrogenInfoIOAPIC *hydrogen_ioapic = (HydrogenInfoIOAPIC *) (HYDROGEN_MEMORY + HYDROGEN_INFO_IOAPIC);
const s8 *HydrogenStrings = (const s8 *) (HYDROGEN_MEMORY + HYDROGEN_INFO_STRINGS);

