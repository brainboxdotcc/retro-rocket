#include <kernel.h>
#include "regex.h"
#include "musl_regex/musl_regex.h"

struct regex_prog {
	regex_t re;                    /* compiled TRE/musl pattern */
	buddy_allocator_t *allocator;
	char err[MAX_STRINGLEN];       /* last error (compile/exec) */
};

static inline void re_set_err(struct regex_prog *P, const char *msg) {
	strlcpy(P->err, msg, sizeof(P->err));
}

static inline void re_set_err_code(struct regex_prog *P, int code, const regex_t *rxopt) {
	if (code == 0) {
		P->err[0] = '\0';
		return;
	}
	/* translate TRE/musl regcomp/regexec codes */
	regerror(code, rxopt, P->err, sizeof(P->err));
}

int regex_compile(buddy_allocator_t *allocator, struct regex_prog **out, const uint8_t *pat, size_t pat_len) {
	if (!out || !pat) {
		return RE_EINVAL;
	}
	*out = NULL;

	struct regex_prog *P = buddy_malloc(allocator, sizeof(*P));
	if (!P) {
		*out = NULL;
		return RE_EMEM;
	}

	memset(P, 0, sizeof(*P));
	P->allocator = allocator;
	P->err[0] = '\0';

	/* null-terminate pattern copy for regcomp */
	char *pattern = buddy_malloc(allocator, pat_len + 1);
	if (!pattern) {
		re_set_err(P, "out of memory");
		*out = P;
		return RE_EMEM;
	}

	memcpy(pattern, pat, pat_len);
	pattern[pat_len] = '\0';

	/* extended ERE; no submatches needed */
	int cflags = REG_EXTENDED | REG_NOSUB;

	int rc = regcomp(&P->re, pattern, cflags);
	buddy_free(allocator, pattern);

	if (rc != 0) {
		re_set_err_code(P, rc, NULL);   /* compile-time message */
		*out = P;
		return RE_EINVAL;
	}

	*out = P;
	return RE_OK;
}

void regex_free(struct regex_prog *p) {
	if (!p) {
		return;
	}

	regfree(&p->re);
	buddy_free(p->allocator, p);
}

int regex_exec_slice(const struct regex_prog *p, const uint8_t *hay, size_t hay_len, size_t start_off, struct regex_match *m, size_t *next_off) {
	if (!p || !hay || !m || !next_off) {
		return RE_EINVAL;
	}

	/* clear result and any previous error */
	m->matched = false;
	m->start_offset = 0;
	m->end_offset = 0;
	/* cast away const to write error text; safe here */
	((struct regex_prog *) p)->err[0] = '\0';

	if (start_off >= hay_len) {
		*next_off = hay_len;
		return RE_NOMATCH;
	}

	/* NUL-terminated by contract: safe to pass suffix pointer */
	const char *subject = (const char *) hay + start_off;

	/* honour true beginning-of-line; end-of-line is the NUL of the suffix */
	int flags = 0;
	if (start_off > 0) flags |= REG_NOTBOL;

	/* nmatch=0, pmatch=NULL because we don't need offsets */
	int rc = regexec(&p->re, subject, 0, NULL, flags);

	if (rc == REG_OK) {
		m->matched = true;
		*next_off = hay_len; /* done for yes/no queries */
		return RE_OK;
	}

	if (rc != REG_NOMATCH) {
		/* hard failure: capture a message */
		re_set_err_code((struct regex_prog *) p, rc, &p->re);
		*next_off = start_off;
		return RE_EINVAL;
	}

	if (start_off + 1 < hay_len) {
		*next_off = start_off + 1; /* cooperative scan */
		return RE_AGAIN;
	}

	*next_off = hay_len;
	return RE_NOMATCH;
}

const char *regex_last_error(const struct regex_prog *p) {
	if (!p || !p->err[0]) {
		return "";
	}
	return p->err;
}

void regex_selftest(void) {
	dprintf("==== regex_selftest ====\n");
	struct {
		const char *pattern;
		const char *hay;
		int expect;     /* 1 = match expected, 0 = no match expected */
	} cases[] = {
		{"A",         "A",          1},
		{"A",         "B",          0},
		{"ABC",       "ZABCZ",      1},
		{"ABC",       "ABZC",       0},
		{"A.*C",      "AXYZC",      1},
		{"A.*C",      "A123",       0},
		{"^A",        "A",          1},
		{"^A",        "BA",         0},
		{"A$",        "A",          1},
		{"A$",        "AB",         0},
		{"B*",        "AAAA",       1},   /* empty match */
		{"AB|CD",     "CD",         1},
		{"AB|CD",     "XY",         0},
		{"[0-9]+",    "foo123bar",  1},
		{"[0-9]+",    "foobar",     0},
		{"HELLO",     "hello",      0},
		{"(?i)HELLO", "hello",      0},   /* TRE lacks (?i), expect fail */
		{"A+",        "AAA",        1},
		{"A+",        "BBB",        0},
		{"X.*Y",      "X123Y",      1},
		{"X.*Y",      "XY",         1},
		{"X.*Y",      "XZ",         0},
		{"AB.*Z$",    "CRAB BALLZ", 1},
		{NULL, NULL,                0}
	};

	buddy_allocator_t alloc;
	buddy_init(&alloc, 5, 23, 23);

	for (int i = 0; cases[i].pattern; i++) {
		struct regex_prog *prog = NULL;
		int rc = regex_compile(&alloc, &prog, (const uint8_t *) cases[i].pattern, strlen(cases[i].pattern));
		if (rc != RE_OK || !prog) {
			dprintf("[%02d] COMPILE FAIL '%s' : %s\n",
				i, cases[i].pattern,
				prog ? regex_last_error(prog) : "no handle");
			if (prog) {
				regex_free(prog);
			}
			continue;
		}

		struct regex_match m = {0};
		size_t next_off = 0;
		int result = RE_AGAIN;
		size_t off = 0;

		while (result == RE_AGAIN) {
			result = regex_exec_slice(prog, (const uint8_t *) cases[i].hay, strlen(cases[i].hay), off, &m, &next_off);
			off = next_off;
		}

		int matched = (result == RE_OK) && m.matched;
		dprintf("[%02d] /%s/ on \"%s\" => %s (expected %s)%s%s\n", i, cases[i].pattern, cases[i].hay, matched ? "MATCH" : "NO MATCH",
			cases[i].expect ? "MATCH" : "NO MATCH", (result == RE_EINVAL && regex_last_error(prog)[0]) ? " : " : "",
			(result == RE_EINVAL && regex_last_error(prog)[0]) ? regex_last_error(prog) : ""
		);
		regex_free(prog);
	}
	buddy_destroy(&alloc);
	dprintf("==== end regex_selftest ====\n");
}
