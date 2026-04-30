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

static int memory_trace_group_compare(const void *a, const void *b)
{
	const memory_trace_leak_group_t *ga = a;
	const memory_trace_leak_group_t *gb = b;

	if (ga->total_allocated < gb->total_allocated) {
		return 1;
	}

	if (ga->total_allocated > gb->total_allocated) {
		return -1;
	}

	return 0;
}

static bool memory_trace_same_group(memory_trace_leak_group_t *g, memory_trace_entry_t *e)
{
	if (g->trace_count != e->trace_count) {
		return false;
	}

	return memcmp(g->trace_addresses, e->trace_addresses, e->trace_count * sizeof(uintptr_t)) == 0;
}

void memory_trace_dump_leaks(memory_trace_owner_type_t owner_type, void *owner)
{
	static memory_trace_leak_group_t groups[MEMORY_TRACE_MAX_LEAK_GROUPS];

	uint64_t flags;
	uint64_t total_leaks = 0;
	uint64_t total_requested = 0;
	uint64_t total_allocated = 0;
	uint64_t dropped_groups = 0;
	size_t group_count = 0;
	uint64_t now = get_ticks();

	memset(groups, 0, sizeof(groups));

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

			total_leaks++;
			total_requested += e->requested_size;
			total_allocated += e->allocated_size;

			bool found = false;

			for (size_t g = 0; g < group_count; g++) {
				if (memory_trace_same_group(&groups[g], e)) {
					groups[g].count++;
					groups[g].total_requested += e->requested_size;
					groups[g].total_allocated += e->allocated_size;

					if (e->ticks < groups[g].oldest_ticks) {
						groups[g].oldest_ticks = e->ticks;
					}

					found = true;
					break;
				}
			}

			if (found) {
				continue;
			}

			if (group_count >= MEMORY_TRACE_MAX_LEAK_GROUPS) {
				dropped_groups++;
				continue;
			}

			memory_trace_leak_group_t *g = &groups[group_count++];

			g->count = 1;
			g->requested_size = e->requested_size;
			g->allocated_size = e->allocated_size;
			g->total_requested = e->requested_size;
			g->total_allocated = e->allocated_size;
			g->oldest_ticks = e->ticks;
			g->trace_count = e->trace_count;

			if (e->trace_count) {
				memcpy(g->trace_addresses, e->trace_addresses, e->trace_count * sizeof(uintptr_t));
			}
		}
	}

	unlock_spinlock_irq(&trace_lock, flags);

	if (!total_leaks) {
		return;
	}

	qsort(groups, group_count, sizeof(memory_trace_leak_group_t), memory_trace_group_compare);

	dprintf("memory_trace: %s owner=%p leaked %lu allocation(s), req=%lu alloc=%lu groups=%lu dropped_groups=%lu\n", owner_type == memory_trace_owner_buddy ? "buddy" : "kmalloc", owner, total_leaks, total_requested, total_allocated, group_count, dropped_groups);

	for (size_t g = 0; g < group_count; g++) {
		memory_trace_leak_group_t *group = &groups[g];

		dprintf("memory_trace: group #%lu count=%lu req_each=%lu alloc_each=%lu req_total=%lu alloc_total=%lu oldest_age=%lu ticks\n", g, group->count, group->requested_size, group->allocated_size, group->total_requested, group->total_allocated, now - group->oldest_ticks);

		for (size_t f = 0; f < group->trace_count; f++) {
			uint64_t offset = 0;
			const char *mname = NULL;
			const char *sname = NULL;
			uintptr_t addr = group->trace_addresses[f];

			if (module_addr_to_symbol(addr, &mname, &sname, &offset)) {
				dprintf("memory_trace:   #%lu %s:%s()+0%08lx [0x%lx]\n", f, mname, sname, offset, addr);
			} else {
				const char *name = findsymbol(addr, &offset);
				dprintf("memory_trace:   #%lu %s()+0%08lx [0x%lx]\n", f, name ? name : "[???]", offset, addr);
			}
		}
	}
}

#endif

