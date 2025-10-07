#pragma once
#include <stddef.h>
#include <stdint.h>

/* result codes */
enum re_res {
	RE_OK = 0,
	RE_NOMATCH = 1,
	RE_AGAIN = -10001,
	RE_EINVAL = -10002,
	RE_ECODE = -10003,
	RE_EMEM = -10004
};

struct regex_opts {
	size_t max_code;      /* hard cap on compiled instructions (0 = no cap) */
	size_t max_threads;   /* cap on live VM threads (0 = default 4096) */
};

struct regex_prog;

struct regex_match {
	int matched;          /* 0/1 */
	size_t so;            /* start offset (bytes) */
	size_t eo;            /* end offset (bytes, one past) */
};

/* compile a counted, byte-only pattern */
int regex_compile(buddy_allocator_t* allocator, struct regex_prog **out, const uint8_t *pat, size_t pat_len, const struct regex_opts *opts);

/* free program */
void regex_free(struct regex_prog *p);

/* run a co-op slice. examine at most `fuel` VM steps.
   resume scanning from `start_off` on RE_AGAIN using returned *next_off. */
int regex_exec_slice(const struct regex_prog *p, const uint8_t *hay, size_t hay_len, size_t start_off, size_t fuel, struct regex_match *m, size_t *next_off);
