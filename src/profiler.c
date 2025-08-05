/**
 * @file profiler.c
 * @brief Retro Rocket kernel profiling subsystem.
 *
 * This module implements lightweight function profiling for Retro Rocket,
 * using GCC's `-finstrument-functions` hooks to measure cycle counts and
 * call relationships. Results are dumped in Callgrind-compatible format
 * for analysis in tools such as KCachegrind.
 *
 * Note that enabling profiling will make the whole OS 25-50x slower!
 * The hooks for it get placed *everywhere* even in critical hot paths.
 *
 * ## Overview
 * - Hooks: `__cyg_profile_func_enter()` / `__cyg_profile_func_exit()`
 *   are inserted automatically around every C function call.
 * - Each function has a `profile_entry` storing total cycles and call counts.
 * - Each call relationship (parent -> child) has a `profile_edge` with call
 *   counts and cumulative cycles.
 * - A fixed-depth stack tracks active function calls to attribute cycles
 *   correctly on exit.
 * - Output is streamed over COM1 as a `callgrind.out` log, which can be
 *   opened directly in KCachegrind for visualisation.
 *
 * ## Usage
 * 1. Build the kernel with `PROFILE_KERNEL=ON` to enable instrumentation.
 * 2. Allocate memory for `PROFILE_MAX_FUNCS` and `PROFILE_MAX_EDGES`, then
 *    call `profile_init()` during startup.
 * 3. Trigger `profile_dump()` manually (e.g. via hotkey) or automatically
 *    on panic to flush results to COM1.
 * 4. Run QEMU with `-serial file:callgrind.out` and open the log in
 *    KCachegrind.
 *
 * ## Notes
 * - This code is excluded from profiling (`no_instrument_function`) to
 *   prevent recursion.
 * - Serial I/O functions are also excluded so that dumping does not
 *   interfere with measurement.
 *
 * @ingroup profiler
 */

#ifdef PROFILE_KERNEL

#include <kernel.h>

#define PROFILE_STACK_DEPTH 256

/**
 * @brief Look up a symbol name from an address.
 *
 * Used when dumping results to translate function pointers into
 * human-readable names.
 *
 * @param address Address to resolve.
 * @param offset  Optional offset into the symbol, may be NULL.
 * @return const char* Name of the symbol, or NULL if not found.
 */
const char* findsymbol(uint64_t address, uint64_t* offset) __attribute__((no_instrument_function));

/** @brief Global table of profiled functions. */
static profile_entry* profile_table = NULL;
/** @brief Count of functions stored in the profile table. */
static size_t profile_entry_count = 0;

/** @brief Global table of call edges between functions. */
static profile_edge* edge_table = NULL;
/** @brief Count of edges stored in the edge table. */
static size_t edge_count = 0;

/**
 * @brief Call stack frame used to measure function entry/exit.
 */
struct call_frame {
	void *fn;              /**< Function pointer. */
	uint64_t enter_time;   /**< Timestamp (TSC) at function entry. */
};
static struct call_frame call_stack[PROFILE_STACK_DEPTH];
static int call_sp = 0;

/**
 * @brief Find or add a function entry in the profile table.
 *
 * @param fn Function pointer.
 * @return profile_entry* Pointer to the entry, or NULL if table full.
 */
static profile_entry *profile_find_or_add(void *fn) __attribute__((no_instrument_function));

/**
 * @brief Find or add an edge in the edge table.
 *
 * @param parent Parent function pointer.
 * @param child  Child function pointer.
 * @return profile_edge* Pointer to the edge, or NULL if table full.
 */
static profile_edge *edge_find_or_add(void *parent, void *child) __attribute__((no_instrument_function));

/**
 * @brief Find an existing function entry or create a new one.
 *
 * Iterates the function profile table to look for an existing entry
 * matching the given function pointer. If not found and space remains,
 * a new entry is allocated and initialised with zero counters.
 *
 * @param fn Function pointer to look up.
 * @return profile_entry* Pointer to the entry, or NULL if the table is full.
 */
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

/**
 * @brief Find an existing edge entry or create a new one.
 *
 * Searches the edge table for a parent->child relationship. If not found
 * and space remains, a new edge is allocated and initialised with zero
 * counters. Each edge represents calls and cycles from one function
 * (parent) into another (child).
 *
 * @param parent Pointer to the parent function.
 * @param child  Pointer to the child function.
 * @return profile_edge* Pointer to the edge, or NULL if the table is full.
 */
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

/**
 * @brief Initialise profiler tables.
 *
 * Caller must allocate memory for both function and edge tables
 * and pass them in. Tables are cleared to zero.
 *
 * @param pre_allocated_funcs Memory for function table.
 * @param pre_allocated_edges Memory for edge table.
 */
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

/**
 * @brief Compiler hook called on function entry.
 *
 * Pushes a call frame onto the stack and records a parent->child
 * call edge if a parent exists.
 *
 * @param this_fn  Function being entered.
 * @param call_site Caller address (unused).
 */
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

/**
 * @brief Compiler hook called on function exit.
 *
 * Pops the call frame, updates the function’s total cycles
 * and call count, and attributes cycles to the parent edge.
 *
 * @param this_fn  Function being exited.
 * @param call_site Caller address (unused).
 */
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

/**
 * @brief Dump profiling results in Callgrind-compatible format.
 *
 * Emits all recorded profiling data (functions and call edges) over COM1
 * in a format directly loadable by KCachegrind. Each function is reported
 * once as a `fn=` block, followed by its self-cost in cycles, and any
 * outgoing call edges as `cfn=` entries with call counts and edge costs.
 *
 * ## Notes
 * - Symbol resolution is attempted via findsymbol(). If no name is found,
 *   raw addresses are printed instead.
 * - The dump can be triggered manually (CTRL+SHIFT+ALT+P) and will outout
 *   automatically on panic.
 * - Serial output is be redirected to a file to capture the profile log.
 */
__attribute__((no_instrument_function)) void profile_dump(void) {
	if (!profile_table) {
		return;
	}
	serial_printf(COM1, "version: 1\n");
	serial_printf(COM1, "creator: RetroRocketProfiler\n");
	serial_printf(COM1, "events: Cycles\n\n");

	for (size_t i = 0; i < profile_entry_count; i++) {
		profile_entry *e = &profile_table[i];
		if (!e->fn) {
			continue;
		}

		uint64_t offset;
		const char *name = findsymbol((uint64_t)e->fn, &offset);
		serial_printf(COM1, "fn=%s\n", name ? name : "unknown");

		// function self-cost
		serial_printf(COM1, "1 %lu\n", (uint64_t)e->total_cycles);

		// child edges
		for (size_t j = 0; j < edge_count; j++) {
			profile_edge *edge = &edge_table[j];
			if (edge->parent == e->fn) {
				const char *cname = findsymbol((uint64_t)edge->child, &offset);
				serial_printf(COM1, "cfn=%s\n", cname ? cname : "unknown");
				serial_printf(COM1, "calls=%lu 1\n", (uint64_t)edge->calls);
				serial_printf(COM1, "1 %lu\n", (uint64_t)edge->total_cycles);
			}
		}

		// terminate block
		serial_printf(COM1, "\n");
	}
}

#endif /* PROFILE_KERNEL */
