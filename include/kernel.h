/**
 * @file kernel.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#if !defined(__GNUC__)
	#error "Retro Rocket is built with GCC only."
#endif
#if !defined(__x86_64__) || (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
	#error "Bit-field layout assumed: x86_64 little-endian."
#endif

#define KSTACK_SIZE  (1 * 1024 * 1024)   /* 1 MB */
#define KSTACK_MASK  (~(KSTACK_SIZE - 1ULL))

#define kprintf(fmt, ...) \
	printf(fmt, ##__VA_ARGS__)

#define aprintf(fmt, ...) \
    do { \
        kprintf(fmt, ##__VA_ARGS__); \
        dprintf(fmt, ##__VA_ARGS__); \
    } while (0)

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <limine.h>
#include "gdt.h"
#include "idt.h"
#include "spinlock.h"
#include "rwlock.h"
#include "cv.h"
#include "printf.h"
#include "hashmap.h"
#include "vector.h"
#include "map.h"
#include "random.h"
#include "video.h"
#include "string.h"
#include "io.h"
#include "memcpy.h"
#include "apic.h"
#include "ioapic.h"
#include "timer.h"
#include "kmalloc.h"
#include "pci.h"
#include "maths.h"
#include "stdlib.h"
#include "devicename.h"
#include "interrupt.h"
#include "ahci.h"
#include "ata.h"
#include "nvme.h"
#include "virtio-block.h"
#include "filesystem.h"
#include "partition.h"
#include "ramdisk.h"
#include "iso9660.h"
#include "devfs.h"
#include "fat32.h"
#include "debugger.h"
#include "errorhandler.h"
#include "keyboard.h"
#include "input.h"
#include "clock.h"
#include "lapic_timer.h"
#include "basic.h"
#include "basic/tokenizer.h"
#include "taskswitch.h"
#include "acpi.h"
#include "net.h"
#include "arp.h"
#include "ip.h"
#include "ethernet.h"
#include "tcp.h"
#include "icmp.h"
#include "udp.h"
#include "dhcp.h"
#include "dns.h"
#include "drawing.h"
#include "fpu.h"
#include "buddy_allocator.h"
#include "retrofs.h"
#include "serial.h"
#include "debug_ringbuffer.h"
#include "module.h"
#include "audio.h"
#include "tls.h"
#include "usb_core.h"
#include "usb_xhci.h"
#include "usb_hid.h"
#include "usb_msc.h"
#include "regex.h"

#define assert(expr, message) if (!(expr)) { \
	kprintf("Assertion failure at %s:%d: %s\n", __FILE__, __LINE__, message); \
	__asm__ volatile("int3"); }

struct reqset {
	const uintptr_t *ptrs;
	size_t count;
};

/**
 * @brief Called on network up
 */
void network_up();

/**
 * @brief Called on network down
 */
void network_down();

/**
 * @brief Verify that Limine's GDT and CR3 are sane, and provide
 * the expected identity map. Also record the value of gdtr.base
 * and CR3 so we can exclude it from BLR reclaim.
 */
void validate_limine_page_tables_and_gdt(void);

/**
 * @brief Collects and returns all bootloader response pointers we must preserve.
 *
 * @details
 * Builds a `reqset` structure containing the runtime addresses of all Limine
 * request/response pairs and other critical early-boot pointers (e.g. GDT base,
 * CR3). These addresses are used during heap initialisation to ensure that
 * **Bootloader Reclaimable** memory ranges containing live data structures are
 * not erroneously freed back into the general allocator.
 *
 * By centralising this list, the heap code can simply check each BLR span for
 * membership and decide whether to keep or reclaim it.
 *
 * @return
 *  A `reqset` with:
 *   - `count` = number of tracked pointers.
 *   - `ptrs[]` = array of the addresses to preserve.
 *
 * @note
 *  - The values in this set are not compile-time constants; they are filled
 *    at runtime once the bootloader has populated its responses.
 *  - Call this only after the Limine hand-off has occurred, typically during
 *    or just before `init_heap()`.
 */
struct reqset request_addresses(void);

#ifdef PROFILE_KERNEL
	typedef struct profile_entry {
		void *fn;
		uint64_t total_cycles;
		uint64_t calls;
	} profile_entry;

	typedef struct profile_edge {
		void *parent;
		void *child;
		uint64_t calls;
		uint64_t total_cycles;
	} profile_edge;

	#define PROFILE_MAX_FUNCS 8192
	#define PROFILE_MAX_EDGES 32768

	void profile_dump(void);
	void profile_init(uint8_t* pre_allocated_funcs, uint8_t* pre_allocated_edges);
#endif

void entropy_irq_event(void);
