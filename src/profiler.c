#ifdef PROFILE_KERNEL

#include <kernel.h>     // for rdtsc, kmalloc, etc.

#define PROFILE_STACK_DEPTH 256

const char* findsymbol(uint64_t address, uint64_t* offset) __attribute__((no_instrument_function));

static profile_entry* profile_table = NULL;
static size_t profile_entry_count = 0;

static profile_edge* edge_table = NULL;
static size_t edge_count = 0;

struct call_frame {
	void *fn;
	uint64_t enter_time;
};
static struct call_frame call_stack[PROFILE_STACK_DEPTH];
static int call_sp = 0;

static profile_entry *profile_find_or_add(void *fn) __attribute__((no_instrument_function));
static profile_edge *edge_find_or_add(void *parent, void *child) __attribute__((no_instrument_function));

/* Find or add a function entry */
static profile_entry *profile_find_or_add(void *fn) {
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

/* Find or add an edge */
static profile_edge *edge_find_or_add(void *parent, void *child) {
	for (size_t i = 0; i < edge_count; i++) {
		if (edge_table[i].parent == parent && edge_table[i].child == child) {
			return &edge_table[i];
		}
	}
	if (edge_count < PROFILE_MAX_EDGES) {
		profile_edge *e = &edge_table[edge_count++];
		e->parent = parent;
		e->child = child;
		e->calls = 0;
		e->total_cycles = 0;
		return e;
	}
	return NULL;
}

/* Called at kernel startup: caller allocates memory for profile + edge tables */
__attribute__((no_instrument_function)) void profile_init(uint8_t* pre_allocated_funcs, uint8_t* pre_allocated_edges) {
	profile_table = (profile_entry*)pre_allocated_funcs;
	edge_table = (profile_edge*)pre_allocated_edges;
	/* Intentionally NOT using memcpy here, as it is profiled */
	if (profile_table) {
		for (uint64_t n = 0; n < sizeof(profile_entry) * PROFILE_MAX_FUNCS; ++n) {
			pre_allocated_funcs[n] = 0;
		}
		profile_entry_count = 0;
	}
	if (edge_table) {
		for (uint64_t n = 0; n < sizeof(profile_edge) * PROFILE_MAX_EDGES; ++n) {
			pre_allocated_edges[n] = 0;
		}
		edge_count = 0;
	}
}

__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *this_fn, void *call_site) {
	if (!profile_table) {
		return;
	}
	uint64_t t = rdtsc();
	if (call_sp < PROFILE_STACK_DEPTH) {
		/* record edge from parent -> this_fn */
		if (call_sp > 0) {
			void *parent = call_stack[call_sp - 1].fn;
			profile_edge *edge = edge_find_or_add(parent, this_fn);
			if (edge) {
				edge->calls++;
			}
		}
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
			uint64_t delta = (t - enter);
			e->total_cycles += delta;
			e->calls++;
			/* attribute to parent edge as well */
			if (call_sp > 0) {
				void *parent = call_stack[call_sp - 1].fn;
				profile_edge *edge = edge_find_or_add(parent, fn);
				if (edge) {
					edge->total_cycles += delta;
				}
			}
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
	serial_printf(COM1, "events: Cycles\n\n");

	for (size_t i = 0; i < profile_entry_count; i++) {
		profile_entry *e = &profile_table[i];
		if (!e->fn) continue;

		uint64_t offset;
		const char *name = findsymbol((uint64_t)e->fn, &offset);
		serial_printf(COM1, "fn=%s\n", name ? name : "unknown");

		// function self-cost
		serial_printf(COM1, "1 %llu\n", (unsigned long long)e->total_cycles);

		// child edges
		for (size_t j = 0; j < edge_count; j++) {
			profile_edge *edge = &edge_table[j];
			if (edge->parent == e->fn) {
				const char *cname = findsymbol((uint64_t)edge->child, &offset);
				serial_printf(COM1, "cfn=%s\n", cname ? cname : "unknown");
				serial_printf(COM1, "calls=%llu 1\n", (unsigned long long)edge->calls);
				serial_printf(COM1, "1 %llu\n", (unsigned long long)edge->total_cycles);
			}
		}

		// terminate block
		serial_printf(COM1, "\n");
	}
}

#endif /* PROFILE_KERNEL */
