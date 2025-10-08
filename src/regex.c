#include <kernel.h>
#include "regex.h"
#include "musl_regex/musl_regex.h"

/* ===== Internal program object ===== */
struct regex_prog {
	regex_t        re;          /* compiled TRE/musl pattern */
	buddy_allocator_t *allocator;

	/* soft budgets for visibility */
	size_t max_code;
	size_t max_threads;
};

/* ===== Compile ===== */
int regex_compile(buddy_allocator_t *allocator,
		  struct regex_prog **out,
		  const uint8_t *pat, size_t pat_len,
		  const struct regex_opts *opts)
{
	if (!out || !pat) {
		return RE_EINVAL;
	}

	*out = NULL;

	struct regex_prog *P = buddy_malloc(allocator, sizeof *P);
	if (!P) {
		return RE_EMEM;
	}

	memset(P, 0, sizeof *P);
	P->allocator   = allocator;
	P->max_code    = opts ? opts->max_code    : 0;
	P->max_threads = opts ? opts->max_threads : 0;

	/* null-terminate pattern copy for regcomp */
	char *pattern = buddy_malloc(allocator, pat_len + 1);
	if (!pattern) {
		buddy_free(allocator, P);
		return RE_EMEM;
	}

	memcpy(pattern, pat, pat_len);
	pattern[pat_len] = '\0';

	int cflags = REG_EXTENDED | REG_NOSUB;

	int rc = regcomp(&P->re, pattern, cflags);
	buddy_free(allocator, pattern);

	if (rc != 0) {
		buddy_free(allocator, P);
		return RE_EINVAL;
	}

	*out = P;
	return RE_OK;
}

/* ===== Free ===== */
void regex_free(struct regex_prog *p)
{
	if (!p) {
		return;
	}

	regfree(&p->re);
	buddy_free(p->allocator, p);
}

int regex_exec_slice(const struct regex_prog *p,
		     const uint8_t *hay, size_t hay_len,
		     size_t start_off, size_t fuel,
		     struct regex_match *m,
		     size_t *next_off)
{
	if (!p || !hay || !m || !next_off) return RE_EINVAL;

	m->matched = 0; m->so = 0; m->eo = 0;

	if (start_off >= hay_len) {
		*next_off = hay_len;
		return RE_NOMATCH;
	}

	/* NUL-terminated by contract: safe to pass suffix pointer. */
	const char *subject = (const char *)hay + start_off;

	/* nmatch=0, pmatch=NULL because we don't need offsets */
	int rc = regexec(&p->re, subject, 0, NULL, 0);

	if (rc == REG_OK) {           /* success is 0 in your tree */
		m->matched = 1;           /* fill for callers that peek at it */
		*next_off = hay_len;      /* we're done for yes/no queries */
		return RE_OK;
	}

	if (start_off + 1 < hay_len) {
		*next_off = start_off + 1;  /* cooperative scan */
		return RE_AGAIN;
	}

	*next_off = hay_len;
	return RE_NOMATCH;
}

/*
 * Quick unit test for musl-based regex layer.
 * Builds, executes, and dumps results using dprintf().
 */
void regex_selftest(void)
{
	dprintf("==== regex_selftest ====\n");

	struct {
		const char *pattern;
		const char *hay;
		int expect;     /* 1 = match expected, 0 = no match expected */
	} cases[] = {
		{ "A",         "A",            1 },
		{ "A",         "B",            0 },
		{ "ABC",       "ZABCZ",        1 },
		{ "ABC",       "ABZC",         0 },
		{ "A.*C",      "AXYZC",        1 },
		{ "A.*C",      "A123",         0 },
		{ "^A",        "A",            1 },
		{ "^A",        "BA",           0 },
		{ "A$",        "A",            1 },
		{ "A$",        "AB",           0 },
		{ "B*",        "AAAA",         1 },   /* empty match */
		{ "AB|CD",     "CD",           1 },
		{ "AB|CD",     "XY",           0 },
		{ "[0-9]+",    "foo123bar",    1 },
		{ "[0-9]+",    "foobar",       0 },
		{ "HELLO",     "hello",        0 },
		{ "(?i)HELLO", "hello",        0 },   /* TRE lacks (?i), expect fail */
		{ "A+",        "AAA",          1 },
		{ "A+",        "BBB",          0 },
		{ "X.*Y",      "X123Y",        1 },
		{ "X.*Y",      "XY",           1 },
		{ "X.*Y",      "XZ",           0 },
		{ NULL, NULL, 0 }
	};

	buddy_allocator_t alloc;
	buddy_init(&alloc, 5, 23, 23);
	struct regex_opts opts = { .max_code = 16384, .max_threads = 1024 };

	for (int i = 0; cases[i].pattern; i++) {
		struct regex_prog *prog = NULL;
		int rc = regex_compile(&alloc, &prog,
				       (const uint8_t *)cases[i].pattern,
				       strlen(cases[i].pattern),
				       &opts);
		if (rc != RE_OK || !prog) {
			dprintf("[%02d] COMPILE FAIL '%s'\n", i, cases[i].pattern);
			continue;
		}

		struct regex_match m = {0};
		size_t next_off = 0;
		int result = RE_AGAIN;
		size_t off = 0;

		/* Run cooperatively until terminal state */
		while (result == RE_AGAIN) {
			result = regex_exec_slice(prog,
						  (const uint8_t *)cases[i].hay,
						  strlen(cases[i].hay),
						  off,
						  5000,
						  &m,
						  &next_off);
			off = next_off;
		}

		int matched = (result == RE_OK) && m.matched;

		dprintf("[%02d] /%s/ on \"%s\" => %s (expected %s)\n",
			i,
			cases[i].pattern,
			cases[i].hay,
			matched ? "MATCH" : "NO MATCH",
			cases[i].expect ? "MATCH" : "NO MATCH");

		regex_free(prog);
	}
	buddy_destroy(&alloc);

	dprintf("==== end regex_selftest ====\n");
}