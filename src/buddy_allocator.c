#include <buddy_allocator.h>
#include <stddef.h>
#include <kernel.h>

#define BUDDY_MAX_ORDER 29
#define BUDDY_MIN_ORDER 6

static inline size_t order_size(int order) {
	return (size_t)1 << order;
}

static inline size_t block_offset(buddy_region_t *region, void *ptr) {
	if (!region) {
		return 0;
	}

	uintptr_t base = (uintptr_t)region->pool;
	uintptr_t p = (uintptr_t)ptr;

	size_t off = p - base;

	/* clamp into region space */
	off &= (region->size - 1);

	return off;
}

static inline void *buddy_of(buddy_region_t *region, void *ptr, int order) {
	if (!region) {
		return 0;
	}

	size_t off = block_offset(region, ptr);
	size_t size = order_size(order);

	size_t buddy_off = off ^ size;

	/* ensure result stays within region */
	buddy_off &= (region->size - 1);

	return (uint8_t *)region->pool + buddy_off;
}

static inline size_t align_up(size_t x, size_t align) {
	return (x + align - 1) & ~(align - 1);
}

static inline int size_to_order(buddy_region_t *region, size_t size) {
	if (!region) {
		return 0;
	}
	size = align_up(size, 1 << region->min_order);
	int order = region->min_order;
	while ((1UL << order) < size) {
		order++;
	}
	return order;
}

void buddy_init(buddy_allocator_t *alloc, int min_order, int max_order, int grow_order) {
	if (!alloc) {
		dprintf("buddy_init: called with a null struct\n");
		return;
	}
	if (min_order < BUDDY_MIN_ORDER || max_order > BUDDY_MAX_ORDER || grow_order > max_order || min_order > max_order) {
		dprintf("buddy_init: invalid order configuration min=%d max=%d grow=%d\n", min_order, max_order, grow_order);
		return;
	}
	alloc->regions = NULL;
	alloc->active_region = NULL;
	alloc->min_order = min_order;
	alloc->max_order = max_order;
	alloc->grow_order = grow_order;
	alloc->current_bytes = 0;
	alloc->peak_bytes = 0;
}

static buddy_region_t *buddy_grow(buddy_allocator_t *alloc) {
	if (!alloc) {
		return NULL;

	}

	int grow = alloc->grow_order;
	if (grow > alloc->max_order) {
		grow = alloc->max_order;
	}

	size_t size = 1UL << grow;
	void *pool = kmalloc(size);
	if (!pool) {
		return NULL;
	}

	buddy_region_t *region = (buddy_region_t *)kmalloc(sizeof(buddy_region_t));
	if (!region) {
		kfree_null(&pool);
		return NULL;
	}

	region->pool = pool;
	region->size = size; /* added: track region size */
	region->min_order = alloc->min_order;
	region->max_order = alloc->max_order;
	region->next = NULL;

	for (int i = 0; i <= alloc->max_order; i++) {
		region->free_lists[i] = NULL;
	}

	// one big free block
	region->free_lists[grow] = (buddy_block_t *)pool;
	region->free_lists[grow]->next = NULL;

	// link it in
	if (!alloc->regions) {
		alloc->regions = region;
	} else {
		buddy_region_t *last = alloc->regions;
		while (last->next) last = last->next;
		last->next = region;
	}
	alloc->active_region = region;
	return region;
}

void *buddy_malloc(buddy_allocator_t *alloc, size_t size) {
	if (!alloc || size == 0) {
		dprintf("buddy_malloc: failed to allocate %lu bytes (bad parameters)\n", size);
		return NULL;
	}

	if (size > ((1UL << alloc->max_order) - sizeof(buddy_header_t))) {
		dprintf("buddy_malloc: request too large to fit in a single region (%lu bytes)\n", size);
		return NULL;
	}

	buddy_region_t *region = alloc->active_region;
	if (!region) {
		region = buddy_grow(alloc);
		if (!region) {
			dprintf("buddy_malloc: failed to allocate %lu bytes (unable to grow)\n", size);
			return NULL;
		}
	}

	while (region) {
		int order = size_to_order(region, size + sizeof(buddy_header_t));

		if (order > region->max_order) {
			region = region->next;
			continue;
		}

		int cur = order;

		// find suitable block in this region
		while (cur <= region->max_order && region->free_lists[cur] == NULL) {
			cur++;
		}

		if (cur <= region->max_order) {
			// got a block: pop from free list
			buddy_block_t *block = region->free_lists[cur];
			region->free_lists[cur] = block->next;

			// split down to required order
			while (cur > order) {
				cur--;
				buddy_block_t *buddy = (buddy_block_t *)((uint8_t *)block + order_size(cur));
				buddy->next = region->free_lists[cur];
				region->free_lists[cur] = buddy;
			}

			// set allocation header
			buddy_header_t *header = (buddy_header_t *)block;
			header->order  = order;
			header->region = region;

			void *ret = (void *)(header + 1);

			// update global counters
			size_t alloc_size = order_size(order);
			alloc->current_bytes += alloc_size;
			if (alloc->current_bytes > alloc->peak_bytes) {
				alloc->peak_bytes = alloc->current_bytes;
			}
			return ret;
		}

		// no block here -> try another region
		if (!region->next) {
			region = buddy_grow(alloc);
		} else {
			region = region->next;
		}
	}

	dprintf("buddy_malloc: failed to allocate %lu bytes (full)\n", size);
	return NULL; // completely out of memory
}

void buddy_free(buddy_allocator_t *alloc, const void *ptr) {
	if (!ptr || !alloc) {
		return;
	}

	buddy_header_t *header = (buddy_header_t *)ptr - 1;
	int order = header->order;
	buddy_region_t *region = header->region;
	buddy_block_t *block = (buddy_block_t *)header;

	size_t alloc_size = order_size(order);
	if (alloc->current_bytes >= alloc_size) {
		alloc->current_bytes -= alloc_size;
	} else {
		alloc->current_bytes = 0; // safety clamp
	}

	int merged;
	do {
		merged = 0;

		if (order >= region->max_order) {
			break;
		}

		void *buddy = buddy_of(region, block, order);

		uintptr_t start = (uintptr_t)region->pool;
		uintptr_t end = start + region->size;

		if ((uintptr_t)buddy < start || (uintptr_t)buddy >= end) {
			break;
		}

		buddy_block_t **prev = &region->free_lists[order];
		buddy_block_t *cur = region->free_lists[order];

		while (cur) {
			if (cur == (buddy_block_t *)buddy) {
				// unlink buddy from free list
				*prev = cur->next;

				// merge to lower address
				if (block > (buddy_block_t *)buddy) {
					block = (buddy_block_t *)buddy;
				}

				order++;
				merged = 1;
				break;
			}
			prev = &cur->next;
			cur = cur->next;
		}
	} while (merged);

	// insert final (possibly merged) block
	block->next = region->free_lists[order];
	region->free_lists[order] = block;
}


void buddy_destroy(buddy_allocator_t *alloc) {
	if (!alloc) {
		return;
	}
	buddy_region_t *r = alloc->regions;
	while (r) {
		buddy_region_t *next = r->next;

		kfree_null(&r->pool);  // free the pool and null it
		kfree_null(&r);        // free the region struct itself and null that pointer

		r = next;              // use the saved "next" to continue traversal
	}
	alloc->regions = NULL;
	alloc->active_region = NULL;
	alloc->current_bytes = 0;
	alloc->peak_bytes = 0;
}

char *buddy_strdup(buddy_allocator_t *alloc, const char *s) {
	if (!s || !alloc) {
		return NULL;
	}
	size_t len = strlen(s) + 1; // include NUL
	char *copy = (char *)buddy_malloc(alloc, len);
	if (!copy) {
		return NULL;
	}
	return memcpy(copy, s, len);
}

void *buddy_realloc(buddy_allocator_t *alloc, void *ptr, size_t size) {
	if (!alloc) {
		return NULL;
	}
	if (!ptr) {
		// realloc(NULL, size): malloc
		return buddy_malloc(alloc, size);
	}
	if (size == 0) {
		// realloc(ptr, 0): free
		buddy_free(alloc, ptr);
		return NULL;
	}

	// Find old size from header
	buddy_header_t *header = (buddy_header_t *)ptr - 1;
	int old_order = header->order;
	size_t old_total = order_size(old_order);
	size_t old_payload = old_total - sizeof(buddy_header_t);

	// Required order for new size
	int new_order = size_to_order(header->region, size + sizeof(buddy_header_t));

	if (new_order == old_order) {
		// Same block size: no change
		return ptr;
	} else if (new_order < old_order) {
		// Shrinking: split block and free remainder
		header->order = new_order;
		// Carve off the extra as a buddy block
		int cur = old_order;
		// repeatedly split until the leftover is the right order
		while (cur > new_order) {
			cur--;
			buddy_block_t *buddy = (buddy_block_t *)((uint8_t *)header + order_size(cur));
			buddy->next = header->region->free_lists[cur];
			header->region->free_lists[cur] = buddy;
		}

		return ptr;
	} else {
		// Growing: allocate + copy + free
		void *new_ptr = buddy_malloc(alloc, size);
		if (!new_ptr) {
			return NULL; // OOM
		}
		memcpy(new_ptr, ptr, old_payload);
		buddy_free(alloc, ptr);
		return new_ptr;
	}
}

void* buddy_calloc(buddy_allocator_t* alloc, size_t num, size_t size) {
	if (!alloc) {
		return NULL;
	}
	void* p = buddy_malloc(alloc, num * size);
	if (p) {
		memset(p, 0, num * size);
	}
	return p;
}
