#ifndef IOAPIC_H
#define IOAPIC_H

void ioapic_register_write(u32 index, u32 value, HydrogenInfoIOAPIC *ioapic);
void ioapic_redir_set(u32 gsi, u32 vector, u32 del_mode, u32 dest_mode, u32 intpol, u32 trigger_mode, u32 mask);
void ioapic_redir_get(u32 gsi, u32* vector, u32* del_mode, u32* dest_mode, u32* intpol, u32* trigger_mode, u32* mask, u32* destination);
void ioapic_redir_unmask(u32 gsi);

#endif
