#ifdef PROFILE_KERNEL

#include <kernel.h>     // for rdtsc, kmalloc, etc.

#define PROFILE_STACK_DEPTH 256

const char* findsymbol(uint64_t address, uint64_t* offset) __attribute__((no_instrument_function));

static struct profile_entry* profile_table = NULL;
static size_t profile_entry_count = 0;

struct call_frame {
	void *fn;
	uint64_t enter_time;
};
static struct call_frame call_stack[PROFILE_STACK_DEPTH];
static int call_sp = 0;

static struct profile_entry *profile_find_or_add(void *fn) __attribute__((no_instrument_function));

/* Find existing entry or allocate a new one */
static struct profile_entry *profile_find_or_add(void *fn) {
	for (size_t i = 0; i < profile_entry_count; i++) {
		if (profile_table[i].fn == fn) {
			return &profile_table[i];
		}
	}
	if (profile_entry_count < PROFILE_MAX_FUNCS) {
		profile_entry *e = &profile_table[profile_entry_count++];
		e->fn = fn;
		e->total_cycles = 0;
		e->calls = 0;
		return e;
	}
	return NULL; // out of slots
}

/* Called at kernel startup */
__attribute__((no_instrument_function)) void profile_init(uint8_t* pre_allocated) {
	profile_table = (profile_entry*)pre_allocated;
	if (profile_table) {
		for (uint64_t n = 0; n < sizeof(profile_entry) * PROFILE_MAX_FUNCS; ++n) {
			pre_allocated[n] = 0;
		}
		profile_entry_count = 0;
	}
}

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *this_fn, void *call_site) {
	if (!profile_table) {
		return;
	}
	uint64_t t = rdtsc();
	if (call_sp < PROFILE_STACK_DEPTH) {
		call_stack[call_sp].fn = this_fn;
		call_stack[call_sp].enter_time = t;
		call_sp++;
	}
}

__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void *this_fn, void *call_site) {
	if (!profile_table) {
		return;
	}
	uint64_t t = rdtsc();
	if (call_sp > 0) {
		call_sp--;
		void *fn = call_stack[call_sp].fn;
		uint64_t enter = call_stack[call_sp].enter_time;
		profile_entry *e = profile_find_or_add(fn);
		if (e) {
			e->total_cycles += (t - enter);
			e->calls++;
		}
	}
}

/* Dump results in Callgrind-compatible format */
__attribute__((no_instrument_function)) void profile_dump(void) {
	if (!profile_table) {
		return;
	}
	serial_printf(COM1, "version: 1\n");
	serial_printf(COM1, "creator: RetroRocketProfiler\n");
	serial_printf(COM1, "events: Cycles\n");

	for (size_t i = 0; i < profile_entry_count; i++) {
		profile_entry *e = &profile_table[i];
		if (!e->fn) {
			continue;
		}
		uint64_t offset;
		const char *name = findsymbol((uint64_t) e->fn, &offset);
		if (!name) {
			serial_printf(COM1, "fn=0x%p\n", e->fn);
		} else {
			serial_printf(COM1, "fn=%s\n", name);
		}
		serial_printf(COM1, "1 %lu\n", (uint64_t) (e->total_cycles));
	}
}

#endif /* PROFILE_KERNEL */
