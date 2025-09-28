#include "allocator.h"

static size_t allocator_alignment = 0;
static allocator_header *allocator_head = 0;

static void allocator_insert_region(allocator_header *h) {
	/* insert by address to keep list monotonic increasing */
	if (allocator_head == 0 || (uintptr_t)h < (uintptr_t)allocator_head) {
		h->next = allocator_head;
		allocator_head = h;
		return;
	}

	allocator_header *cur = allocator_head;
	while (cur->next && (uintptr_t)cur->next < (uintptr_t)h) {
		cur = cur->next;
	}

	h->next = cur->next;
	cur->next = h;
}

/* merge only when two blocks are physically adjacent */
static void allocator_coalesce_all(void) {
	allocator_header *cur = allocator_head;
	while (cur) {
		allocator_header *n = cur->next;
		if (n && cur->free && n->free) {
			uint8_t *cur_end = (uint8_t *)cur + sizeof(allocator_header) + cur->size;
			if (cur_end == (uint8_t *)n) {
				cur->size += sizeof(allocator_header) + n->size;
				cur->next = n->next;
				continue; /* try to merge the new neighbour as well */
			}
		}
		cur = cur->next;
	}
}

void allocator_init(void *heap, size_t size, size_t alignment) {
	allocator_alignment = alignment;

	allocator_head = (allocator_header *) heap;
	allocator_head->size = size - sizeof(allocator_header);
	allocator_head->free = true;
	allocator_head->next = 0;
}

void *allocator_alloc(size_t size) {
	allocator_header *current = allocator_head;
	while (current) {
		if (current->free && current->size >= size) {
			size_t total_size = size + sizeof(allocator_header);

			if (current->size >= total_size + sizeof(allocator_header) + allocator_alignment) {
				allocator_header *next = (allocator_header *) ((uint8_t *) current + total_size);
				next->size = current->size - total_size;
				next->free = true;
				next->next = current->next;

				current->size = size;
				current->next = next;
			}

			current->free = false;
			return (void *) (current + 1);
		}
		current = current->next;
	}
	return 0; /* OOM */
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
	size_t s = header->size;

	/* Coalesce only truly adjacent free blocks */
	allocator_coalesce_all();
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

	allocator_header *h = (allocator_header *)region;
	h->size = size - sizeof(allocator_header);
	h->free = true;
	h->next = 0;

	allocator_insert_region(h);
	allocator_coalesce_all(); /* merge if this abuts an existing region */
	return true;
}
