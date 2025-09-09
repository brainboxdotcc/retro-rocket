#include "tinyalloc.h"

static size_t ta_alignment = 0;
static ta_header *ta_head = 0;

static void ta_insert_region(ta_header *h) {
	/* insert by address to keep list monotonic increasing */
	if (ta_head == 0 || (uintptr_t)h < (uintptr_t)ta_head) {
		h->next = ta_head;
		ta_head = h;
		return;
	}

	ta_header *cur = ta_head;
	while (cur->next && (uintptr_t)cur->next < (uintptr_t)h) {
		cur = cur->next;
	}

	h->next = cur->next;
	cur->next = h;
}

/* merge only when two blocks are physically adjacent */
static void ta_coalesce_all(void) {
	ta_header *cur = ta_head;
	while (cur) {
		ta_header *n = cur->next;
		if (n && cur->free && n->free) {
			uint8_t *cur_end = (uint8_t *)cur + sizeof(ta_header) + cur->size;
			if (cur_end == (uint8_t *)n) {
				cur->size += sizeof(ta_header) + n->size;
				cur->next = n->next;
				continue; /* try to merge the new neighbour as well */
			}
		}
		cur = cur->next;
	}
}

void ta_init(void *heap, size_t size, size_t alignment) {
	ta_alignment = alignment;

	ta_head = (ta_header *) heap;
	ta_head->size = size - sizeof(ta_header);
	ta_head->free = true;
	ta_head->next = 0;
}

void *ta_alloc(size_t size) {
	ta_header *current = ta_head;
	while (current) {
		if (current->free && current->size >= size) {
			size_t total_size = size + sizeof(ta_header);

			if (current->size >= total_size + sizeof(ta_header) + ta_alignment) {
				ta_header *next = (ta_header *) ((uint8_t *) current + total_size);
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

size_t ta_free(void *ptr) {
	if (!ptr) {
		return 0;
	}
	ta_header *header = ((ta_header *) ptr) - 1;
	if (header->free) {
		preboot_fail("Double free");
	}
	header->free = true;
	size_t s = header->size;

	/* Coalesce only truly adjacent free blocks */
	ta_coalesce_all();
	return s;
}

size_t ta_usable_size(void *ptr) {
	if (!ptr) {
		return 0;
	}
	ta_header *h = ((ta_header *) ptr) - 1;
	return h->size;
}

bool ta_add_region(void *region, size_t size) {
	if (!region || size <= sizeof(ta_header)) {
		return false;
	}

	ta_header *h = (ta_header *)region;
	h->size = size - sizeof(ta_header);
	h->free = true;
	h->next = 0;

	ta_insert_region(h);
	ta_coalesce_all(); /* merge if this abuts an existing region */
	return true;
}
