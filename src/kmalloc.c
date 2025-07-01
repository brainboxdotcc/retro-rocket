#include <kernel.h>
#include <kmalloc.h>
#include <limine.h>
#include <debugger.h>
#include <tinyalloc.h>

#define LOW_HEAP_START 0x800000
#define LOW_HEAP_MAX   0x1400000

#define PAGE_SIZE 0x1000

extern uint64_t k_end;

static uint64_t heapstart = 0;
static uint64_t heaplen   = 0;
static uint64_t heap_pos  = 0;

static uint32_t low_mem_cur = LOW_HEAP_START;

volatile struct limine_memmap_request memory_map_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

void preboot_fail(const char* msg) {
    setforeground(current_console, COLOUR_LIGHTRED);
    kprintf("PANIC: %s\n", msg);
    backtrace();   // show stack trace
    wait_forever();
}

void init_heap() {
    uint64_t best_len = 0;
    uint64_t best_addr = 0;

    for (uint64_t i = 0; i < memory_map_request.response->entry_count; ++i) {
        struct limine_memmap_entry* entry = memory_map_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > best_len) {
            best_len = entry->length;
            best_addr = entry->base;
        }
    }

    if (!best_addr || best_len < 0x800000)
        preboot_fail("No usable RAM found for heap");

    heapstart = best_addr + LOW_HEAP_MAX;
    heaplen   = best_len  - LOW_HEAP_MAX;
    heap_pos  = heapstart;

    print_heapinfo();

    ta_init(heapstart, heaplen, 8, 4096);
}

void print_heapinfo() {
    kprintf("Heap start: 0x%llx size: %llu bytes\n", heapstart, heaplen);
}

void* kmalloc(uint64_t size) {
    return ta_alloc(size); 
}

void kfree(const void* ptr) {
    if (!ptr) return;

    uintptr_t a = (uintptr_t)ptr;
    if (a >= LOW_HEAP_START && a < LOW_HEAP_MAX) {
        preboot_fail("kfree: tried to free low heap memory — use kfree_low instead!");
    }

    ta_free(ptr);
}


uint32_t kmalloc_low(uint32_t size) {
    uint32_t ret = low_mem_cur;
    if (ret + size >= LOW_HEAP_MAX)
        preboot_fail("kmalloc_low exhausted");
    low_mem_cur += size;
    return ret;
}

void kfree_low(uint32_t addr) {
    (void)addr; /* no-op */
}

void* kcalloc(size_t num, size_t size) {
    void* p = kmalloc(num * size);
    memset(p, 0, num * size);
    return p;
}

void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);

    void* new_ptr = kmalloc(new_size);
    memcpy(new_ptr, ptr, new_size);
    kfree(ptr);
    return new_ptr;
}

uint64_t get_free_memory() {
    uint64_t free = 0;
    return free;
}

uint64_t get_used_memory() {
    return heaplen - get_free_memory();
}

uint64_t get_total_memory() {
    return heaplen;
}

