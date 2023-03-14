#ifndef IOAPIC_H
#define IOAPIC_H

void ioapic_redir_set(u32 gsi, u32 vector, u32 del_mode, u32 dest_mode, u32 intpol, u32 trigger_mode, u32 mask);
void ioapic_redir_get(u32 gsi, u32* vector, u32* del_mode, u32* dest_mode, u32* intpol, u32* trigger_mode, u32* mask, u32* destination);
void ioapic_redir_unmask(u32 gsi);
void ioapic_redir_set_precalculated(u32 gsi, u32 upper, u32 lower);

#endif
