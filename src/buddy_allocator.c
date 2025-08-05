#include <buddy_allocator.h>
#include <stddef.h>
#include <kernel.h>

#define BUDDY_MAX_ORDER 24
#define BUDDY_MIN_ORDER 6

static inline size_t order_size(int order) {
	return (size_t)1 << order;
}

static inline size_t block_offset(buddy_region_t *region, void *ptr) {
	return (uintptr_t)ptr - (uintptr_t)region->pool;
}

static inline void *buddy_of(buddy_region_t *region, void *ptr, int order) {
	size_t off = block_offset(region, ptr);
	size_t size = order_size(order);
	return (uint8_t *)region->pool + (off ^ size);
}

static inline size_t align_up(size_t x, size_t align) {
	return (x + align - 1) & ~(align - 1);
}

static inline int size_to_order(buddy_region_t *region, size_t size) {
	size = align_up(size, 1 << region->min_order);
	int order = region->min_order;
	while ((1UL << order) < size) {
		order++;
	}
	return order;
}

void buddy_init(buddy_allocator_t *alloc, int min_order, int max_order, int grow_order) {
	alloc->regions = NULL;
	alloc->active_region = NULL;
	alloc->min_order = min_order;
	alloc->max_order = max_order;
	alloc->grow_order = grow_order;
	alloc->current_bytes = 0;
	alloc->peak_bytes = 0;
}

static buddy_region_t *buddy_grow(buddy_allocator_t *alloc) {
	size_t size = 1UL << alloc->grow_order;
	void *pool = kmalloc(size);
	if (!pool) return NULL;

	buddy_region_t *region = (buddy_region_t *)kmalloc(sizeof(buddy_region_t));
	if (!region) {
		kfree_null(&pool);
		return NULL;
	}

	region->pool = pool;
	region->min_order = alloc->min_order;
	region->max_order = alloc->max_order;
	region->next = NULL;

	for (int i = 0; i <= alloc->max_order; i++) {
		region->free_lists[i] = NULL;
	}

	// one big free block
	region->free_lists[alloc->grow_order] = (buddy_block_t *)pool;
	region->free_lists[alloc->grow_order]->next = NULL;

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
	if (size == 0) return NULL;

	buddy_region_t *region = alloc->active_region;
	if (!region) {
		region = buddy_grow(alloc);
		if (!region) return NULL;
	}

	while (region) {
		int order = size_to_order(region, size + sizeof(buddy_header_t));
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

		// no block here → try another region
		if (!region->next) {
			region = buddy_grow(alloc);
		} else {
			region = region->next;
		}
	}

	return NULL; // completely out of memory
}

void buddy_free(buddy_allocator_t *alloc, void *ptr) {
	if (!ptr) return;

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
