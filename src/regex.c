#include <kernel.h>
#include "regex.h"

/* =========================
   byte-mode Thompson engine
   ========================= */

enum tok_kind {
	T_END, T_CHAR, T_DOT, T_ALT, T_LP, T_RP,
	T_STAR, T_PLUS, T_QMARK, T_BOL, T_EOL, T_CLASS
};

struct token {
	int k;
	int c;     /* for CHAR: byte value; for CLASS: class index */
};

struct lexer {
	const uint8_t *s;
	size_t n;
	size_t i;
	struct regex_prog *prog;
};

enum rx_op {
	RX_CHAR,   /* c: byte, x: next */
	RX_ANY,    /* x: next */
	RX_CLASS,  /* c: class index, x: next */
	RX_JMP,    /* x: target */
	RX_SPLIT,  /* x: a, y: b */
	RX_MATCH,  /* terminal */
	RX_BOL,    /* x: next, assert start-of-line/subject */
	RX_EOL     /* x: next, assert end-of-line/subject */
};

struct rx_insn {
	int op;
	int c;
	int x;
	int y;
};

struct classset {
	/* simple 256-byte bitmap (1 byte per codepoint, byte-mode) */
	uint8_t map[256];
	int neg; /* 0 normal, 1 negated (^...) */
};

struct regex_prog {
	struct rx_insn *code;
	size_t count;
	size_t cap;
	size_t max_code;
	size_t max_threads;

	struct classset *classes;
	size_t cls_count;
	size_t cls_cap;
	buddy_allocator_t* allocator;
};

static int grow_code(struct regex_prog *p) {
	if (p->count < p->cap) {
		return 1;
	}
	size_t ncap = p->cap == 0 ? 64 : p->cap * 2;
	if (p->max_code > 0 && ncap > p->max_code) {
		ncap = p->max_code;
	}
	if (ncap <= p->cap) {
		return 0;
	}
	void *nbuf = buddy_realloc(p->allocator, p->code, ncap * sizeof(struct rx_insn));
	if (!nbuf) {
		return 0;
	}
	p->code = (struct rx_insn *)nbuf;
	p->cap = ncap;
	return 1;
}

static int emit(struct regex_prog *p, int op, int c, int x, int y) {
	if (p->max_code > 0 && p->count >= p->max_code) {
		return -1;
	}
	if (!grow_code(p)) {
		return -1;
	}
	size_t i = p->count;
	p->code[i].op = op;
	p->code[i].c = c;
	p->code[i].x = x;
	p->code[i].y = y;
	p->count++;
	return (int)i;
}

static int class_new(struct regex_prog *p) {
	if (p->cls_count == p->cls_cap) {
		size_t ncap = p->cls_cap == 0 ? 8 : p->cls_cap * 2;
		void *nb = buddy_realloc(p->allocator, p->classes, ncap * sizeof(struct classset));
		if (!nb) {
			return -1;
		}
		p->classes = (struct classset *)nb;
		p->cls_cap = ncap;
	}
	int idx = (int)p->cls_count;
	memset(&p->classes[idx], 0, sizeof(struct classset));
	p->cls_count++;
	return idx;
}

static int lx_peek(struct lexer *lx) {
	if (lx->i >= lx->n) {
		return -1;
	}
	return lx->s[lx->i];
}

static int lx_get(struct lexer *lx) {
	int ch = lx_peek(lx);
	if (ch >= 0) {
		lx->i++;
	}
	return ch;
}

static int read_escape(struct lexer *lx) {
	int c2 = lx_get(lx);
	if (c2 < 0) {
		return '\\';
	}
	if (c2 == 'n') {
		return 10;
	}
	if (c2 == 't') {
		return 9;
	}
	if (c2 == 'r') {
		return 13;
	}
	return c2;
}

static int add_range(struct classset *cl, int a, int b) {
	if (a > b) {
		int t = a; a = b; b = t;
	}
	for (int v = a; v <= b; v++) {
		cl->map[(uint8_t)v] = 1;
	}
	return 1;
}

static int parse_class(struct lexer *lx, struct token *out)
{
	int idx = class_new(lx->prog);
	if (idx < 0) {
		return 0;
	}
	struct classset *cl = &lx->prog->classes[idx];

	int first = 1;
	int prev = -1;
	int got_char = 0;

	int ch = lx_get(lx);
	if (ch == '^') {
		cl->neg = 1;
	} else {
		lx->i--; /* put back */
	}

	for (;;) {
		ch = lx_get(lx);
		if (ch < 0) {
			return 0; /* unterminated */
		}
		if (ch == ']' && !first) {
			break;
		}
		first = 0;

		int lit;
		if (ch == '\\') {
			lit = read_escape(lx);
		} else {
			lit = ch;
		}

		/* range? only if we have prev, next char exists, and next is not ']' */
		int peek = lx_peek(lx);
		if (peek == '-' && prev == -1) {
			/* possible range start: store lit as prev and consume '-' */
			prev = lit;
			lx_get(lx);
			/* next must exist and not be ']' */
			int nxt = lx_get(lx);
			if (nxt < 0) {
				return 0;
			}
			if (nxt == ']') {
				/* treat '-' as literal, we consumed ']' too early -> error */
				return 0;
			}
			int rhs;
			if (nxt == '\\') {
				rhs = read_escape(lx);
			} else {
				rhs = nxt;
			}
			add_range(cl, prev, rhs);
			prev = -1;
			got_char = 1;
			continue;
		} else {
			/* single literal */
			cl->map[(uint8_t)lit] = 1;
			got_char = 1;
			prev = -1;
			continue;
		}
	}

	if (!got_char) {
		/* empty class '[]' or '[^]' -> treat as always-fail / always-match? choose fail. */
		memset(cl->map, 0, sizeof cl->map);
		cl->neg = 0;
	}

	out->k = T_CLASS;
	out->c = idx;
	return 1;
}

static struct token next_token(struct lexer *lx, int at_start, int need_cat_hint) {
	int ch = lx_get(lx);
	struct token t = { T_END, 0 };
	if (ch < 0) {
		return t;
	}
	if (ch == '\\') {
		t.k = T_CHAR;
		t.c = read_escape(lx);
		return t;
	}
	if (ch == '[') {
		if (!parse_class(lx, &t)) {
			t.k = T_END; /* will propagate as error later */
		}
		return t;
	}
	switch (ch) {
		case '.': t.k = T_DOT; break;
		case '|': t.k = T_ALT; break;
		case '(': t.k = T_LP; break;
		case ')': t.k = T_RP; break;
		case '*': t.k = T_STAR; break;
		case '+': t.k = T_PLUS; break;
		case '?': t.k = T_QMARK; break;
		case '^': t.k = T_BOL; break;
		case '$': t.k = T_EOL; break;
		default:  t.k = T_CHAR; t.c = ch; break;
	}
	return t;
}

/* -------------------------
   shunting-yard to postfix
   ------------------------- */

#define T_CAT 1001

static int prec(int tok) {
	if (tok == T_ALT) {
		return 1;
	}
	if (tok == T_CAT) {
		return 2;
	}
	if (tok == T_STAR || tok == T_PLUS || tok == T_QMARK) {
		return 3;
	}
	return 0;
}

struct pbuf {
	struct token *v;
	size_t n, cap;
};

static int ppush(buddy_allocator_t* alloc, struct pbuf *pb, struct token t) {
	if (pb->n == pb->cap) {
		size_t ncap = pb->cap == 0 ? 64 : pb->cap * 2;
		void *nv = buddy_realloc(alloc, pb->v, ncap * sizeof(struct token));
		if (!nv) {
			return 0;
		}
		pb->v = (struct token *)nv;
		pb->cap = ncap;
	}
	pb->v[pb->n++] = t;
	return 1;
}

struct frag {
	int start;
	int out_a;
};

static int to_postfix(struct regex_prog *prog, const uint8_t *pat, size_t n, struct pbuf *out, int *err)
{
	struct lexer lx = { pat, n, 0, prog };
	struct token opstk[256];
	int top = -1;
	enum tok_kind prev = T_END;
	int at_start = 1;

	for (;;) {
		struct token t = next_token(&lx, at_start, 0);
		if (t.k == T_END) {
			break;
		}

		/* insert implicit concatenation if previous token emits output
		   and next token begins an atom */
		int prev_cat =
			(prev == T_CHAR || prev == T_DOT || prev == T_CLASS ||
			 prev == T_RP || prev == T_STAR || prev == T_PLUS ||
			 prev == T_QMARK || prev == T_EOL);
		int next_atom =
			(t.k == T_CHAR || t.k == T_LP || t.k == T_DOT ||
			 t.k == T_BOL || t.k == T_CLASS);

		if (prev_cat && next_atom) {
			struct token tc = { T_CAT, 0 };
			while (top >= 0 && prec(opstk[top].k) >= prec(T_CAT)) {
				if (!ppush(prog->allocator, out, opstk[top--])) {
					*err = RE_EMEM;
					return 0;
				}
			}
			if (++top >= 256) {
				*err = RE_ECODE;
				return 0;
			}
			opstk[top] = tc;
		}

		/* main token handling */
		if (t.k == T_CHAR || t.k == T_DOT || t.k == T_BOL ||
		    t.k == T_EOL || t.k == T_CLASS) {
			if (!ppush(prog->allocator, out, t)) {
				*err = RE_EMEM;
				return 0;
			}
			prev = t.k;
			at_start = 0;
			continue;
		}

		if (t.k == T_LP) {
			if (++top >= 256) { *err = RE_ECODE; return 0; }
			opstk[top] = t;
			prev = t.k;
			at_start = 1;
			continue;
		}

		if (t.k == T_RP) {
			int found = 0;
			while (top >= 0) {
				if (opstk[top].k == T_LP) {
					found = 1;
					top--;
					break;
				}
				if (!ppush(prog->allocator, out, opstk[top--])) {
					*err = RE_EMEM;
					return 0;
				}
			}
			if (!found) { *err = RE_EINVAL; return 0; }
			prev = t.k;
			at_start = 0;
			continue;
		}

		if (t.k == T_STAR || t.k == T_PLUS || t.k == T_QMARK) {
			if (!ppush(prog->allocator, out, t)) {
				*err = RE_EMEM;
				return 0;
			}
			prev = t.k;
			at_start = 0;
			continue;
		}

		if (t.k == T_ALT) {
			while (top >= 0 && prec(opstk[top].k) >= prec(T_ALT)) {
				if (!ppush(prog->allocator, out, opstk[top--])) {
					*err = RE_EMEM;
					return 0;
				}
			}
			if (++top >= 256) { *err = RE_ECODE; return 0; }
			opstk[top] = t;
			prev = t.k;
			at_start = 1;
			continue;
		}

		prev = t.k;
	}

	while (top >= 0) {
		if (opstk[top].k == T_LP) {
			*err = RE_EINVAL;
			return 0;
		}
		if (!ppush(prog->allocator, out, opstk[top--])) {
			*err = RE_EMEM;
			return 0;
		}
	}
	*err = RE_OK;
	return 1;
}

static int list_append(int a, int b, struct regex_prog *p) {
	if (a == 0) {
		return b;
	}
	if (b == 0) {
		return a;
	}
	int i = a;
	for (;;) {
		int next = p->code[i].x;
		if (next >= 0) {
			p->code[i].x = - (b + 1);
			return a;
		} else {
			i = -next - 1;
		}
	}
}

static void list_patch(int a, int target, struct regex_prog *p) {
	while (a >= 0) {
		int next = p->code[a].x;
		p->code[a].x = target;
		if (next < 0) {
			a = -next - 1;
		} else {
			break;
		}
	}
}

static int compile_postfix(struct regex_prog *prog, const struct token *pf, size_t pn) {
	struct frag st[256];
	int sp = -1;

	for (size_t i = 0; i < pn; i++) {
		struct token t = pf[i];

		if (t.k == T_CHAR) {
			int i1 = emit(prog, RX_CHAR, t.c, -1, 0);
			if (i1 < 0) {
				return RE_ECODE;
			}
			struct frag f = { i1, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_DOT) {
			int i1 = emit(prog, RX_ANY, 0, -1, 0);
			if (i1 < 0) {
				return RE_ECODE;
			}
			struct frag f = { i1, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_CLASS) {
			int i1 = emit(prog, RX_CLASS, t.c, -1, 0);
			if (i1 < 0) {
				return RE_ECODE;
			}
			struct frag f = { i1, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_BOL) {
			int i1 = emit(prog, RX_BOL, 0, -1, 0);
			if (i1 < 0) {
				return RE_ECODE;
			}
			struct frag f = { i1, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_EOL) {
			int i1 = emit(prog, RX_EOL, 0, -1, 0);
			if (i1 < 0) {
				return RE_ECODE;
			}
			struct frag f = { i1, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_CAT) {
			if (sp < 1) {
				return RE_EINVAL;
			}
			struct frag b = st[sp--];
			struct frag a = st[sp--];
			list_patch(a.out_a, b.start, prog);
			struct frag f = { a.start, b.out_a };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_ALT) {
			if (sp < 1) {
				return RE_EINVAL;
			}
			struct frag b = st[sp--];
			struct frag a = st[sp--];
			int i1 = emit(prog, RX_SPLIT, 0, a.start, b.start);
			if (i1 < 0) {
				return RE_ECODE;
			}
			int out = list_append(a.out_a, b.out_a, prog);
			struct frag f = { i1, out };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_STAR) {
			if (sp < 0) {
				return RE_EINVAL;
			}
			struct frag a = st[sp--];
			int i1 = emit(prog, RX_SPLIT, 0, a.start, -1);
			if (i1 < 0) {
				return RE_ECODE;
			}
			list_patch(a.out_a, i1, prog);
			struct frag f = { i1, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_PLUS) {
			if (sp < 0) {
				return RE_EINVAL;
			}
			struct frag a = st[sp--];
			int i1 = emit(prog, RX_SPLIT, 0, a.start, -1);
			if (i1 < 0) {
				return RE_ECODE;
			}
			list_patch(a.out_a, i1, prog);
			struct frag f = { a.start, i1 };
			st[++sp] = f;
			continue;
		}
		if (t.k == T_QMARK) {
			if (sp < 0) {
				return RE_EINVAL;
			}
			struct frag a = st[sp--];
			int i1 = emit(prog, RX_SPLIT, 0, a.start, -1);
			if (i1 < 0) {
				return RE_ECODE;
			}
			int out = list_append(a.out_a, i1, prog);
			struct frag f = { i1, out };
			st[++sp] = f;
			continue;
		}

		return RE_EINVAL;
	}

	if (sp != 0) {
		return RE_EINVAL;
	}

	int m = emit(prog, RX_MATCH, 0, 0, 0);
	if (m < 0) {
		return RE_ECODE;
	}
	list_patch(st[0].out_a, m, prog);
	return RE_OK;
}

/* -----------------
   compile front-end
   ----------------- */

int regex_compile(buddy_allocator_t* allocator, struct regex_prog **out, const uint8_t *pat, size_t pat_len, const struct regex_opts *opts) {
	if (!out || !pat) {
		return RE_EINVAL;
	}

	struct regex_prog *p = buddy_malloc(allocator, sizeof(*p));
	if (!p) {
		return RE_EMEM;
	}
	p->max_code = opts ? opts->max_code : 0;
	p->max_threads = opts ? opts->max_threads : 0;
	p->allocator = allocator;

	struct pbuf pf = { 0, 0, 0 };
	int perr = RE_OK;
	if (!to_postfix(p, pat, pat_len, &pf, &perr)) {
		buddy_free(p->allocator, p);
		buddy_free(p->allocator, pf.v);
		return perr;
	}

	int rc = compile_postfix(p, pf.v, pf.n);
	buddy_free(p->allocator, pf.v);
	if (rc != RE_OK) {
		buddy_free(p->allocator, p->code);
		buddy_free(p->allocator, p->classes);
		buddy_free(p->allocator, p);
		return rc;
	}

	*out = p;
	return RE_OK;
}

void regex_free(struct regex_prog *p)
{
	if (!p) {
		return;
	}
	buddy_free(p->allocator, p->code);
	buddy_free(p->allocator, p->classes);
	buddy_free(p->allocator, p);
}

/* -----------
   VM runtime
   ----------- */

static int class_test(const struct regex_prog *p, int cls_idx, uint8_t ch)
{
	const struct classset *cl = &p->classes[cls_idx];
	int hit = cl->map[ch] ? 1 : 0;
	if (cl->neg) {
		return hit ? 0 : 1;
	} else {
		return hit;
	}
}

static void add_epsilon(const struct regex_prog *p,
			const uint8_t *hay, size_t hay_len,
			size_t pos, int pc,
			int *list, size_t *ln, size_t lcap,
			uint32_t *seen, uint32_t gen,
			size_t *fuel)
{
	int stk[512];
	int sp = -1;

	if ((size_t)pc >= p->count) {
		return;
	}
	if (seen[pc] == gen) {
		return;
	}
	stk[++sp] = pc;

	while (sp >= 0) {
		if (*fuel == 0) {
			return;
		}
		(*fuel)--;

		int ip = stk[sp--];
		if (seen[ip] == gen) {
			continue;
		}
		seen[ip] = gen;

		struct rx_insn ins = p->code[ip];
		if (ins.op == RX_JMP) {
			if (ins.x >= 0 && (size_t)ins.x < p->count) {
				if (seen[ins.x] != gen) {
					stk[++sp] = ins.x;
				}
			}
			continue;
		}
		if (ins.op == RX_SPLIT) {
			if (ins.x >= 0 && (size_t)ins.x < p->count) {
				if (seen[ins.x] != gen) {
					stk[++sp] = ins.x;
				}
			}
			if (ins.y >= 0 && (size_t)ins.y < p->count) {
				if (seen[ins.y] != gen) {
					stk[++sp] = ins.y;
				}
			}
			continue;
		}
		if (ins.op == RX_BOL) {
			int at_bol = (pos == 0) || (pos > 0 && hay[pos - 1] == '\n');
			if (at_bol) {
				if (ins.x >= 0 && (size_t)ins.x < p->count) {
					if (seen[ins.x] != gen) {
						stk[++sp] = ins.x;
					}
				}
			}
			continue;
		}
		if (ins.op == RX_EOL) {
			int at_eol = (pos == hay_len) || (pos < hay_len && hay[pos] == '\n');
			if (at_eol) {
				if (ins.x >= 0 && (size_t)ins.x < p->count) {
					if (seen[ins.x] != gen) {
						stk[++sp] = ins.x;
					}
				}
			}
			continue;
		}

		if (*ln < lcap) {
			list[(*ln)++] = ip;
		}
	}
}

static int step_vm(const struct regex_prog *p,
		   const uint8_t *hay, size_t hay_len,
		   size_t pos,
		   int *cur, size_t *cn,
		   int *nxt, size_t *nn,
		   uint32_t *seen_cur, uint32_t *seen_nxt,
		   uint32_t gen_cur, uint32_t gen_nxt,
		   size_t *fuel,
		   size_t max_threads)
{
	if (*cn == 0) {
		return 0;
	}

	int matched = 0;

	for (size_t i = 0; i < *cn; i++) {
		if (*fuel == 0) {
			return -1;
		}
		(*fuel)--;

		int ip = cur[i];
		if ((size_t)ip >= p->count) {
			continue;
		}
		struct rx_insn ins = p->code[ip];

		if (ins.op == RX_MATCH) {
			matched = 1;
			continue;
		}

		if (pos >= hay_len) {
			continue;
		}

		int ok = 0;
		if (ins.op == RX_CHAR) {
			ok = (hay[pos] == (uint8_t)ins.c);
		} else if (ins.op == RX_ANY) {
			ok = 1;
		} else if (ins.op == RX_CLASS) {
			ok = class_test(p, ins.c, hay[pos]);
		}

		if (ok) {
			add_epsilon(p, hay, hay_len, pos + 1, ins.x,
				    nxt, nn, max_threads,
				    seen_nxt, gen_nxt, fuel);
			if (max_threads > 0 && *nn > max_threads) {
				return -2;
			}
		}
	}

	return matched;
}

int regex_exec_slice(const struct regex_prog *p,
		     const uint8_t *hay, size_t hay_len,
		     size_t start_off, size_t fuel,
		     struct regex_match *m,
		     size_t *next_off)
{
	if (!p || !hay || !m || !next_off) {
		return RE_EINVAL;
	}
	m->matched = 0;
	m->so = 0;
	m->eo = 0;

	if (start_off > hay_len) {
		*next_off = hay_len;
		return RE_NOMATCH;
	}

	size_t cap = p->max_threads ? p->max_threads : 4096;
	int *list_a = (int *)buddy_malloc(p->allocator, sizeof(int) * cap);
	int *list_b = (int *)buddy_malloc(p->allocator, sizeof(int) * cap);
	uint32_t *seen_a = (uint32_t *)buddy_calloc(p->allocator, p->count, sizeof(uint32_t));
	uint32_t *seen_b = (uint32_t *)buddy_calloc(p->allocator, p->count, sizeof(uint32_t));
	if (!list_a || !list_b || !seen_a || !seen_b) {
		buddy_free(p->allocator, list_a);
		buddy_free(p->allocator, list_b);
		buddy_free(p->allocator, seen_a);
		buddy_free(p->allocator, seen_b);
		return RE_EMEM;
	}

	uint32_t gen_a = 1;
	uint32_t gen_b = 1;

	size_t cn = 0, nn = 0;
	size_t pos = start_off;

	add_epsilon(p, hay, hay_len, pos, 0, list_a, &cn, cap, seen_a, gen_a, &fuel);

	size_t so = pos;

	while (pos <= hay_len) {
		int r = step_vm(p, hay, hay_len, pos, list_a, &cn, list_b, &nn, seen_a, seen_b, gen_a, gen_b, &fuel, cap);
		if (r == 1) {
			m->matched = 1;
			m->so = so;
			m->eo = pos;
			buddy_free(p->allocator, list_a);
			buddy_free(p->allocator, list_b);
			buddy_free(p->allocator, seen_a);
			buddy_free(p->allocator, seen_b);
			*next_off = pos;
			return RE_OK;
		}
		if (r == -1) {
			buddy_free(p->allocator, list_a);
			buddy_free(p->allocator, list_b);
			buddy_free(p->allocator, seen_a);
			buddy_free(p->allocator, seen_b);
			*next_off = pos;
			return RE_AGAIN;
		}
		if (r == -2) {
			buddy_free(p->allocator, list_a);
			buddy_free(p->allocator, list_b);
			buddy_free(p->allocator, seen_a);
			buddy_free(p->allocator, seen_b);
			return RE_EMEM;
		}

		pos++;

		int *tmp_list = list_a; list_a = list_b; list_b = tmp_list;
		uint32_t *tmp_seen = seen_a; seen_a = seen_b; seen_b = tmp_seen;
		size_t tmp_n = cn; (void)tmp_n; cn = nn; nn = 0;

		gen_b++;
		if (gen_b == 0) {
			memset(seen_b, 0, p->count * sizeof(uint32_t));
			gen_b = 1;
		}

		if (cn == 0) {
			gen_a++;
			if (gen_a == 0) {
				memset(seen_a, 0, p->count * sizeof(uint32_t));
				gen_a = 1;
			}
			add_epsilon(p, hay, hay_len, pos, 0, list_a, &cn, cap, seen_a, gen_a, &fuel);
			so = pos;
		}

		if (pos > hay_len) {
			break;
		}
	}

	buddy_free(p->allocator, list_a);
	buddy_free(p->allocator, list_b);
	buddy_free(p->allocator, seen_a);
	buddy_free(p->allocator, seen_b);
	*next_off = hay_len;
	return RE_NOMATCH;
}
