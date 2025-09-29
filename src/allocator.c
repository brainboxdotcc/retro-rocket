#include "allocator.h"

static size_t allocator_alignment = 0;
static allocator_header *allocator_head = 0;
/* Next-fit search hint: reduces average scan length without changing semantics */
static allocator_header *allocator_cursor = 0;

/* Optional: minimum remainder to allow a split (header + alignment) */
#ifndef ALLOCATOR_MIN_SPLIT
#define ALLOCATOR_MIN_SPLIT (sizeof(allocator_header) + allocator_alignment)
#endif

static inline size_t allocator_round_up(size_t v, size_t a) {
	if (a == 0) {
		return v;
	}
	return (v + (a - 1)) & ~(a - 1);
}

static void allocator_insert_region(allocator_header *h) {
	/* insert by address to keep list monotonic increasing */
	if (allocator_head == 0 || (uintptr_t) h < (uintptr_t) allocator_head) {
		h->next = allocator_head;
		allocator_head = h;
		return;
	}

	allocator_header *cur = allocator_head;
	while (cur->next && (uintptr_t) cur->next < (uintptr_t) h) {
		cur = cur->next;
	}

	h->next = cur->next;
	cur->next = h;
}

/* Full sweep coalesce */
[[maybe_unused]] static void allocator_coalesce_all(void) {
	allocator_header *cur = allocator_head;
	while (cur) {
		allocator_header *n = cur->next;
		if (n && cur->free && n->free) {
			uint8_t *cur_end = (uint8_t *) cur + sizeof(allocator_header) + cur->size;
			if (cur_end == (uint8_t *) n) {
				cur->size += sizeof(allocator_header) + n->size;
				cur->next = n->next;
				continue; /* try to merge the new neighbour as well */
			}
		}
		cur = cur->next;
	}
}

/* Find the node immediately prior to h (O(n)) */
static allocator_header *allocator_find_prev(const allocator_header *h) {
	allocator_header *cur = allocator_head;
	allocator_header *prev = 0;
	while (cur && cur != h) {
		prev = cur;
		cur = cur->next;
	}
	return prev;
}

/* Local coalesce around a specific free block (no full sweep) */
static void allocator_coalesce_local(allocator_header *h) {
	allocator_header *prev = allocator_find_prev(h);
	allocator_header *next = h->next;

	/* forward merge */
	if (next && next->free) {
		uint8_t *end = (uint8_t *) h + sizeof(*h) + h->size;
		if (end == (uint8_t *) next) {
			h->size += sizeof(*next) + next->size;
			h->next = next->next;
			next = h->next;
		}
	}

	/* backward merge */
	if (prev && prev->free) {
		uint8_t *end = (uint8_t *) prev + sizeof(*prev) + prev->size;
		if (end == (uint8_t *) h) {
			prev->size += sizeof(*h) + h->size;
			prev->next = h->next;
			h = prev;
		}
	}

	/* keep the search cursor helpful */
	if (allocator_cursor == 0 || allocator_cursor == next) {
		allocator_cursor = h;
	}
}

/* Next-fit: scan from a hint, then wrap to head once if needed */
static allocator_header *allocator_find_suitable(allocator_header *start, size_t size) {
	allocator_header *cur = start;
	while (cur) {
		if (cur->free && cur->size >= size) {
			return cur;
		}
		cur = cur->next;
	}
	return 0;
}

void allocator_init(void *heap, size_t size, size_t alignment) {
	allocator_alignment = alignment;

	allocator_head = (allocator_header *) heap;
	allocator_head->size = size - sizeof(allocator_header);
	allocator_head->free = true;
	allocator_head->next = 0;

	allocator_cursor = allocator_head;
}

void *allocator_alloc(size_t size) {
	size = allocator_round_up(size, (allocator_alignment ? allocator_alignment : sizeof(void *)));

	allocator_header *start = (allocator_cursor ? allocator_cursor : allocator_head);
	allocator_header *current = allocator_find_suitable(start, size);
	if (!current && start != allocator_head) {
		current = allocator_find_suitable(allocator_head, size); /* wrap once */
	}
	if (!current) {
		return NULL; /* OOM */
	}

	size_t total_size = size + sizeof(allocator_header);

	/* Only split if the remainder can hold a header plus aligned payload */
	if (current->size >= total_size + ALLOCATOR_MIN_SPLIT) {
		allocator_header *next = (allocator_header *) ((uint8_t *) current + total_size);
		next->size = current->size - total_size;
		next->free = true;
		next->next = current->next;

		current->size = size;
		current->next = next;
	}

	current->free = false;

	/* advance next-fit cursor */
	allocator_cursor = (current->next ? current->next : allocator_head);

	return (void *) (current + 1);
}

size_t allocator_free(void *ptr) {
	if (!ptr) {
		return 0;
	}
	allocator_header *header = ((allocator_header *) ptr) - 1;
	if (header->free) {
		preboot_fail("Double free");
	}
	header->free = true;
	size_t s = header->size; /* preserve existing return-value semantics */

	/* Local coalesce only (adjacent blocks) */
	allocator_coalesce_local(header);
	return s;
}

size_t allocator_usable_size(void *ptr) {
	if (!ptr) {
		return 0;
	}
	allocator_header *h = ((allocator_header *) ptr) - 1;
	return h->size;
}

bool allocator_add_region(void *region, size_t size) {
	if (!region || size <= sizeof(allocator_header)) {
		return false;
	}

	allocator_header *h = (allocator_header *) region;
	h->size = size - sizeof(allocator_header);
	h->free = true;
	h->next = 0;

	allocator_insert_region(h);
	/* Merge only if this abuts neighbours; avoids a full sweep */
	allocator_coalesce_local(h);

	if (allocator_cursor == 0) {
		allocator_cursor = allocator_head;
	}

	return true;
}
