#pragma once
#include <kernel.h>
#include <stdatomic.h>

typedef struct simple_cv {
	int waiting;
} simple_cv_t;

static inline void simple_cv_init(simple_cv_t *cv) {
	atomic_store(&cv->waiting, 0);
}

// called by consumer
static inline void simple_cv_wait(simple_cv_t *cv) {
	int id = atomic_fetch_add(&cv->waiting, 1) + 1;

	// spin until producer broadcasts
	while (atomic_load(&cv->waiting) >= id) {
		__builtin_ia32_pause();
	}
}

// called by producer
static inline void simple_cv_broadcast(simple_cv_t *cv) {
	atomic_store(&cv->waiting, 0);
}
