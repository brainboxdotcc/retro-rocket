#include <buddy_allocator.h>
#include <stddef.h>
#include <stdint.h>

// ---------------- Config ----------------
#define BUDDY_MAX_ORDER 20   // 1 << 20 = 1MB max block
#define BUDDY_MIN_ORDER 12   // 1 << 12 = 4KB min block

// ---------------- Helpers ----------------
static inline size_t order_size(int order) {
	return (size_t)1 << order;
}

static inline size_t align_up(size_t x, size_t align) {
	return (x + align - 1) & ~(align - 1);
}

static inline int size_to_order(buddy_allocator_t *alloc, size_t size) {
	size = align_up(size, 1 << alloc->min_order);
	int order = alloc->min_order;
	while ((1UL << order) < size) {
		order++;
	}
	return order;
}

static inline size_t block_offset(buddy_allocator_t *alloc, void *ptr) {
	return (uintptr_t)ptr - (uintptr_t)alloc->pool;
}

static inline void *buddy_of(buddy_allocator_t *alloc, void *ptr, int order) {
	size_t off = block_offset(alloc, ptr);
	size_t size = order_size(order);
	return (uint8_t *)alloc->pool + (off ^ size);
}

// ---------------- API ----------------

// Initialise an allocator on top of backing_pool of size (1 << max_order)
void buddy_init(buddy_allocator_t *alloc, void *backing_pool,
		int min_order, int max_order) {
	alloc->pool = (uint8_t *)backing_pool;
	alloc->min_order = min_order;
	alloc->max_order = max_order;
	for (int i = 0; i <= max_order; i++) {
		alloc->free_lists[i] = NULL;
	}
	// One giant free block initially
	alloc->free_lists[max_order] = (buddy_block_t *)alloc->pool;
	alloc->free_lists[max_order]->next = NULL;
}

// Alloc from this allocator
void *buddy_malloc(buddy_allocator_t *alloc, size_t size) {
	if (size == 0) return NULL;
	int order = size_to_order(alloc, size + sizeof(int));
	int cur = order;

	// find suitable block
	while (cur <= alloc->max_order && alloc->free_lists[cur] == NULL) {
		cur++;
	}
	if (cur > alloc->max_order) return NULL; // OOM

	// pop from free list
	buddy_block_t *block = alloc->free_lists[cur];
	alloc->free_lists[cur] = block->next;

	// split down
	while (cur > order) {
		cur--;
		buddy_block_t *buddy = (buddy_block_t *)((uint8_t *)block + order_size(cur));
		buddy->next = alloc->free_lists[cur];
		alloc->free_lists[cur] = buddy;
	}

	// store order
	int *header = (int *)block;
	*header = order;
	void *ret = (void *)(header + 1);

	// track usage
	size_t alloc_size = order_size(order);
	alloc->current_bytes += alloc_size;
	if (alloc->current_bytes > alloc->peak_bytes) {
		alloc->peak_bytes = alloc->current_bytes;
	}

	return ret;
}

// Free to this allocator
void buddy_free(buddy_allocator_t *alloc, void *ptr) {
	if (!ptr) return;
	int *header = (int *)ptr - 1;
	int order = *header;
	buddy_block_t *block = (buddy_block_t *)header;
	size_t alloc_size = order_size(order);

	// update usage
	if (alloc->current_bytes >= alloc_size) {
		alloc->current_bytes -= alloc_size;
	} else {
		alloc->current_bytes = 0; // safety clamp
	}

	for (;;) {
		if (order >= alloc->max_order) break;

		void *buddy = buddy_of(alloc, block, order);

		// search buddy in free list
		buddy_block_t **prev = &alloc->free_lists[order];
		buddy_block_t *cur = alloc->free_lists[order];
		while (cur) {
			if (cur == (buddy_block_t *)buddy) {
				// remove buddy from list
				*prev = cur->next;
				// merge
				if (block > (buddy_block_t *)buddy)
					block = buddy;
				order++;
				goto merged;
			}
			prev = &cur->next;
			cur = cur->next;
		}
		break; // buddy not free
		merged:;
	}

	block->next = alloc->free_lists[order];
	alloc->free_lists[order] = block;
}
