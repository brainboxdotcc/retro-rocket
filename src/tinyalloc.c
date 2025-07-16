#include "tinyalloc.h"

typedef struct ta_header {
    size_t size;
    bool free;
    struct ta_header* next;
} ta_header;

static void* ta_heap = 0;
static size_t ta_heapsize = 0;
static size_t ta_alignment = 0;
static ta_header* ta_head = 0;

void ta_init(void* heap, size_t size, size_t alignment, size_t pagesize) {
    ta_heap = heap;
    ta_heapsize = size;
    ta_alignment = alignment;

    ta_head = (ta_header*)heap;
    ta_head->size = size - sizeof(ta_header);
    ta_head->free = true;
    ta_head->next = 0;
}

[[maybe_unused]] static void* align_forward(void* ptr, size_t align) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t mod = p % align;
    if (mod) p += (align - mod);
    return (void*)p;
}

void* ta_alloc(size_t size) {
    ta_header* current = ta_head;
    while (current) {
        if (current->free && current->size >= size) {
            size_t total_size = size + sizeof(ta_header);

            if (current->size >= total_size + sizeof(ta_header) + ta_alignment) {
                ta_header* next = (ta_header*)((uint8_t*)current + total_size);
                next->size = current->size - total_size;
                next->free = true;
                next->next = current->next;

                current->size = size;
                current->next = next;
            }

            current->free = false;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    return 0; // OOM
}

void ta_free(void* ptr) {
    if (!ptr) return;

    ta_header* header = ((ta_header*)ptr) - 1;
    header->free = true;

    // Coalesce
    ta_header* current = ta_head;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += sizeof(ta_header) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

