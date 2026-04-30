#ifdef MEMORY_TRACE
#include <kernel.h>
#include <string.h>
#include "memory_trace.h"

static const uint64_t trace_seed0 = 0x736f6d6570736575ULL;
static const uint64_t trace_seed1 = 0x646f72616e646f6dULL;

static memory_trace_entry_t trace_table[MEMORY_TRACE_BUCKETS][MEMORY_TRACE_BUCKET_DEPTH];

static spinlock_t trace_lock = 0;

static uint64_t trace_dropped;
static uint64_t trace_remove_miss;
static uint64_t trace_active;
static uint64_t trace_peak;

static inline size_t trace_hash_ptr(void *ptr)
{
	uint64_t h = hashmap_sip(&ptr, sizeof(ptr), trace_seed0, trace_seed1);
	return (size_t)(h & (MEMORY_TRACE_BUCKETS - 1));
}

void memory_trace_add(void *ptr, void *owner, memory_trace_owner_type_t owner_type, size_t requested_size, size_t allocated_size, const uint64_t *trace_addresses, size_t trace_count) {
	if (!ptr) {
		return;
	}

	uint64_t flags;
	lock_spinlock_irq(&trace_lock, &flags);

	size_t b = trace_hash_ptr(ptr);

	for (size_t i = 0; i < MEMORY_TRACE_BUCKET_DEPTH; i++) {
		memory_trace_entry_t *e = &trace_table[b][i];

		if (!e->used) {
			e->used = true;
			e->ptr = ptr;
			e->owner = owner;
			e->owner_type = owner_type;
			e->requested_size = requested_size;
			e->allocated_size = allocated_size;
			e->ticks = get_ticks();

			e->trace_count = trace_count;
			if (e->trace_count > MEMORY_TRACE_MAX_STACK) {
				e->trace_count = MEMORY_TRACE_MAX_STACK;
			}

			if (trace_addresses && e->trace_count) {
				memcpy(e->trace_addresses, trace_addresses,
				       e->trace_count * sizeof(int64_t));
			}

			trace_active++;
			if (trace_active > trace_peak) {
				trace_peak = trace_active;
			}

			unlock_spinlock_irq(&trace_lock, flags);
			return;
		}
	}

	trace_dropped++;
	unlock_spinlock_irq(&trace_lock, flags);
}

void memory_trace_remove(void *ptr)
{
	if (!ptr) {
		return;
	}

	uint64_t flags;
	lock_spinlock_irq(&trace_lock, &flags);

	size_t b = trace_hash_ptr(ptr);

	for (size_t i = 0; i < MEMORY_TRACE_BUCKET_DEPTH; i++) {
		memory_trace_entry_t *e = &trace_table[b][i];

		if (e->used && e->ptr == ptr) {
			e->used = false;
			trace_active--;
			unlock_spinlock_irq(&trace_lock, flags);
			return;
		}
	}

	trace_remove_miss++;
	unlock_spinlock_irq(&trace_lock, flags);
}

void memory_trace_dump_leaks(memory_trace_owner_type_t owner_type, void *owner)
{
	uint64_t leaks = 0;
	uint64_t bytes = 0;
	uint64_t now = get_ticks();
	uint64_t flags;

	lock_spinlock_irq(&trace_lock, &flags);

	for (size_t b = 0; b < MEMORY_TRACE_BUCKETS; b++) {
		for (size_t i = 0; i < MEMORY_TRACE_BUCKET_DEPTH; i++) {
			memory_trace_entry_t *e = &trace_table[b][i];

			if (!e->used) {
				continue;
			}

			if (e->owner_type != owner_type) {
				continue;
			}

			if (owner && e->owner != owner) {
				continue;
			}

			leaks++;
			bytes += e->allocated_size;

			dprintf("memory_trace: leak type=%s owner=%p ptr=%p req=%lu alloc=%lu age=%lu ticks\n", owner_type == memory_trace_owner_buddy ? "buddy" : "kmalloc", e->owner, e->ptr, e->requested_size, e->allocated_size, now - e->ticks);

			for (size_t f = 0; f < e->trace_count; f++) {
				uint64_t offset = 0;
				const char *mname = NULL;
				const char *sname = NULL;
				uintptr_t addr = e->trace_addresses[f];

				if (module_addr_to_symbol(addr, &mname, &sname, &offset)) {
					dprintf("memory_trace:   #%lu %s:%s()+0x%08lx [0x%lx]\n", f, mname, sname, offset, addr);
				} else {
					const char *name = findsymbol(addr, &offset);
					dprintf("memory_trace:   #%lu %s()+0x%08lx [0x%lx]\n", f, name ? name : "[???]", offset, addr);
				}
			}
		}
	}

	if (leaks) {
		dprintf("memory_trace: type=%s owner=%p leaked %lu allocation(s), %lu bytes\n", owner_type == memory_trace_owner_buddy ? "buddy" : "kmalloc", owner, leaks, bytes);
	}

	unlock_spinlock_irq(&trace_lock, flags);
}

#endif

