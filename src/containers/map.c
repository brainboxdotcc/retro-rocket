#include <kernel.h>

enum rb_colour {
	RB_RED = 0,
	RB_BLACK = 1
};

typedef struct map_soa {
	vector keys;     /* uintptr_t key */
	vector vals;     /* uintptr_t value */
	vector left;     /* size_t index or NIL (stored as uintptr_t) */
	vector right;    /* size_t index or NIL */
	vector parent;   /* size_t index or NIL */
	vector colour;   /* RB_RED / RB_BLACK (stored as uintptr_t) */
} map_soa;

struct map {
	map_soa a;
	vector free_ix;      /* recycled indices (uintptr_t cast of size_t) */
	size_t root;         /* index of root, or NIL */
	size_t count;        /* live elements */
	map_keycmp cmp;
	void *opaque;
};

enum iter_state {
	ITER_OK = 0,
	ITER_END = 1,
	ITER_BEFORE_BEGIN = 2
};

struct map_iter {
	const map *owner;
	size_t i;       /* current index, or NIL */
	int state;      /* enum iter_state */
};

static const size_t NIL = (size_t)-1;

static int rd_colour(const vector *v, size_t i) {
	return (int)vector_get(v, i);
}

static void wr_colour(vector *v, size_t i, int colour) {
	vector_set(v, i, (uintptr_t)colour);
}

static size_t rd_idx(const vector *v, size_t i) {
	return (size_t)vector_get(v, i);
}

static void wr_idx(vector *v, size_t i, size_t value) {
	vector_set(v, i, (uintptr_t)value);
}

static uintptr_t rd_up(const vector *v, size_t i) {
	return vector_get(v, i);
}

static void wr_up(vector *v, size_t i, uintptr_t value) {
	vector_set(v, i, value);
}

/* ---------- SoA utilities ---------- */

static int soa_push_blank(map_soa *a) {
	if (vector_push(&a->keys, 0) != 0) {
		return -1;
	}
	if (vector_push(&a->vals, 0) != 0) {
		return -1;
	}
	if (vector_push(&a->left, (uintptr_t)NIL) != 0) {
		return -1;
	}
	if (vector_push(&a->right, (uintptr_t)NIL) != 0) {
		return -1;
	}
	if (vector_push(&a->parent, (uintptr_t)NIL) != 0) {
		return -1;
	}
	if (vector_push(&a->colour, RB_RED) != 0) {
		return -1;
	}
	return 0;
}

static size_t soa_size(const map_soa *a) {
	return vector_size(&a->keys);
}

map *map_create(map_keycmp cmp, void *opaque) {
	if (cmp == NULL) {
		return NULL;
	}

	map *m = (map *)kmalloc(sizeof(map));
	if (m == NULL) {
		return NULL;
	}

	vector_init(&m->a.keys,   64);
	vector_init(&m->a.vals,   64);
	vector_init(&m->a.left,   64);
	vector_init(&m->a.right,  64);
	vector_init(&m->a.parent, 64);
	vector_init(&m->a.colour, 64);
	vector_init(&m->free_ix,  32);

	m->root = NIL;
	m->count = 0;
	m->cmp = cmp;
	m->opaque = opaque;

	return m;
}

static void soa_free(map_soa *a) {
	vector_free(&a->keys);
	vector_free(&a->vals);
	vector_free(&a->left);
	vector_free(&a->right);
	vector_free(&a->parent);
	vector_free(&a->colour);
}

void map_destroy(map *m) {
	if (m == NULL) {
		return;
	}
	soa_free(&m->a);
	vector_free(&m->free_ix);
	kfree(m);
}

void map_clear(map *m) {
	if (m == NULL) {
		return;
	}
	vector_clear(&m->a.keys);
	vector_clear(&m->a.vals);
	vector_clear(&m->a.left);
	vector_clear(&m->a.right);
	vector_clear(&m->a.parent);
	vector_clear(&m->a.colour);
	vector_clear(&m->free_ix);
	m->root = NIL;
	m->count = 0;
}

size_t map_size(const map *m) {
	if (m == NULL) {
		return 0;
	}
	return m->count;
}

static int node_new(map *m, uintptr_t key, uintptr_t val, size_t parent, size_t *out_i) {
	size_t i;

	if (vector_size(&m->free_ix) != 0) {
		i = (size_t)vector_pop(&m->free_ix);
		wr_up(&m->a.keys, i, key);
		wr_up(&m->a.vals, i, val);
		wr_idx(&m->a.left, i, NIL);
		wr_idx(&m->a.right, i, NIL);
		wr_idx(&m->a.parent, i, parent);
		wr_colour(&m->a.colour, i, RB_RED);
	} else {
		if (soa_push_blank(&m->a) != 0) {
			return -1;
		}
		i = soa_size(&m->a) - 1;
		wr_up(&m->a.keys, i, key);
		wr_up(&m->a.vals, i, val);
		wr_idx(&m->a.left, i, NIL);
		wr_idx(&m->a.right, i, NIL);
		wr_idx(&m->a.parent, i, parent);
		wr_colour(&m->a.colour, i, RB_RED);
	}

	*out_i = i;
	return 0;
}

static void node_recycle(map *m, size_t i) {
	(void)vector_push(&m->free_ix, (uintptr_t)i);
}

static int colour_of(const map *m, size_t i) {
	if (i == NIL) {
		return RB_BLACK;
	}
	return rd_colour(&m->a.colour, i);
}

static size_t minimum_node(const map *m, size_t i) {
	size_t cur = i;
	while (cur != NIL) {
		size_t l = rd_idx(&m->a.left, cur);
		if (l == NIL) {
			break;
		}
		cur = l;
	}
	return cur;
}

static size_t maximum_node(const map *m, size_t i) {
	size_t cur = i;
	while (cur != NIL) {
		size_t r = rd_idx(&m->a.right, cur);
		if (r == NIL) {
			break;
		}
		cur = r;
	}
	return cur;
}

static size_t successor(const map *m, size_t i) {
	if (i == NIL) {
		return NIL;
	}
	size_t r = rd_idx(&m->a.right, i);
	if (r != NIL) {
		return minimum_node(m, r);
	}
	size_t cur = i;
	size_t p = rd_idx(&m->a.parent, cur);
	while (p != NIL && cur == rd_idx(&m->a.right, p)) {
		cur = p;
		p = rd_idx(&m->a.parent, cur);
	}
	return p;
}

static size_t predecessor(const map *m, size_t i) {
	if (i == NIL) {
		return NIL;
	}
	size_t l = rd_idx(&m->a.left, i);
	if (l != NIL) {
		return maximum_node(m, l);
	}
	size_t cur = i;
	size_t p = rd_idx(&m->a.parent, cur);
	while (p != NIL && cur == rd_idx(&m->a.left, p)) {
		cur = p;
		p = rd_idx(&m->a.parent, cur);
	}
	return p;
}

static void rotate_left(map *m, size_t x) {
	size_t y = rd_idx(&m->a.right, x);
	wr_idx(&m->a.right, x, rd_idx(&m->a.left, y));
	if (rd_idx(&m->a.right, x) != NIL) {
		wr_idx(&m->a.parent, rd_idx(&m->a.right, x), x);
	}
	wr_idx(&m->a.parent, y, rd_idx(&m->a.parent, x));
	if (rd_idx(&m->a.parent, x) == NIL) {
		m->root = y;
	} else {
		size_t xp = rd_idx(&m->a.parent, x);
		if (x == rd_idx(&m->a.left, xp)) {
			wr_idx(&m->a.left, xp, y);
		} else {
			wr_idx(&m->a.right, xp, y);
		}
	}
	wr_idx(&m->a.left, y, x);
	wr_idx(&m->a.parent, x, y);
}

static void rotate_right(map *m, size_t x) {
	size_t y = rd_idx(&m->a.left, x);
	wr_idx(&m->a.left, x, rd_idx(&m->a.right, y));
	if (rd_idx(&m->a.left, x) != NIL) {
		wr_idx(&m->a.parent, rd_idx(&m->a.left, x), x);
	}
	wr_idx(&m->a.parent, y, rd_idx(&m->a.parent, x));
	if (rd_idx(&m->a.parent, x) == NIL) {
		m->root = y;
	} else {
		size_t xp = rd_idx(&m->a.parent, x);
		if (x == rd_idx(&m->a.right, xp)) {
			wr_idx(&m->a.right, xp, y);
		} else {
			wr_idx(&m->a.left, xp, y);
		}
	}
	wr_idx(&m->a.right, y, x);
	wr_idx(&m->a.parent, x, y);
}

static void insert_fixup(map *m, size_t z) {
	while (rd_idx(&m->a.parent, z) != NIL && colour_of(m, rd_idx(&m->a.parent, z)) == RB_RED) {
		size_t p = rd_idx(&m->a.parent, z);
		size_t gp = rd_idx(&m->a.parent, p);
		if (p == rd_idx(&m->a.left, gp)) {
			size_t u = rd_idx(&m->a.right, gp);
			if (colour_of(m, u) == RB_RED) {
				wr_colour(&m->a.colour, p, RB_BLACK);
				wr_colour(&m->a.colour, u, RB_BLACK);
				wr_colour(&m->a.colour, gp, RB_RED);
				z = gp;
			} else {
				if (z == rd_idx(&m->a.right, p)) {
					z = p;
					rotate_left(m, z);
				}
				wr_colour(&m->a.colour, rd_idx(&m->a.parent, z), RB_BLACK);
				wr_colour(&m->a.colour, gp, RB_RED);
				rotate_right(m, gp);
			}
		} else {
			size_t u = rd_idx(&m->a.left, gp);
			if (colour_of(m, u) == RB_RED) {
				wr_colour(&m->a.colour, p, RB_BLACK);
				wr_colour(&m->a.colour, u, RB_BLACK);
				wr_colour(&m->a.colour, gp, RB_RED);
				z = gp;
			} else {
				if (z == rd_idx(&m->a.left, p)) {
					z = p;
					rotate_right(m, z);
				}
				wr_colour(&m->a.colour, rd_idx(&m->a.parent, z), RB_BLACK);
				wr_colour(&m->a.colour, gp, RB_RED);
				rotate_left(m, gp);
			}
		}
	}
	if (m->root != NIL) {
		wr_colour(&m->a.colour, m->root, RB_BLACK);
	}
}

static void transplant(map *m, size_t u, size_t v) {
	size_t up = rd_idx(&m->a.parent, u);
	if (up == NIL) {
		m->root = v;
	} else if (u == rd_idx(&m->a.left, up)) {
		wr_idx(&m->a.left, up, v);
	} else {
		wr_idx(&m->a.right, up, v);
	}
	if (v != NIL) {
		wr_idx(&m->a.parent, v, up);
	}
}

static void erase_fixup(map *m, size_t x, size_t x_parent) {
	while ((x == NIL || colour_of(m, x) == RB_BLACK) && x != m->root) {
		if (x_parent != NIL && x == rd_idx(&m->a.left, x_parent)) {
			size_t w = rd_idx(&m->a.right, x_parent);
			if (colour_of(m, w) == RB_RED) {
				wr_colour(&m->a.colour, w, RB_BLACK);
				wr_colour(&m->a.colour, x_parent, RB_RED);
				rotate_left(m, x_parent);
				w = rd_idx(&m->a.right, x_parent);
			}
			if (colour_of(m, rd_idx(&m->a.left, w)) == RB_BLACK &&
			    colour_of(m, rd_idx(&m->a.right, w)) == RB_BLACK) {
				if (w != NIL) {
					wr_colour(&m->a.colour, w, RB_RED);
				}
				x = x_parent;
				x_parent = rd_idx(&m->a.parent, x_parent);
			} else {
				if (colour_of(m, rd_idx(&m->a.right, w)) == RB_BLACK) {
					size_t wl = rd_idx(&m->a.left, w);
					if (wl != NIL) {
						wr_colour(&m->a.colour, wl, RB_BLACK);
					}
					if (w != NIL) {
						wr_colour(&m->a.colour, w, RB_RED);
					}
					rotate_right(m, w);
					w = rd_idx(&m->a.right, x_parent);
				}
				if (w != NIL) {
					wr_colour(&m->a.colour, w, rd_colour(&m->a.colour, x_parent));
				}
				wr_colour(&m->a.colour, x_parent, RB_BLACK);
				size_t wr = (w != NIL) ? rd_idx(&m->a.right, w) : NIL;
				if (wr != NIL) {
					wr_colour(&m->a.colour, wr, RB_BLACK);
				}
				rotate_left(m, x_parent);
				x = m->root;
				x_parent = NIL;
			}
		} else {
			size_t w = (x_parent != NIL) ? rd_idx(&m->a.left, x_parent) : NIL;
			if (colour_of(m, w) == RB_RED) {
				wr_colour(&m->a.colour, w, RB_BLACK);
				if (x_parent != NIL) {
					wr_colour(&m->a.colour, x_parent, RB_RED);
					rotate_right(m, x_parent);
				}
				w = (x_parent != NIL) ? rd_idx(&m->a.left, x_parent) : NIL;
			}
			if (colour_of(m, rd_idx(&m->a.right, w)) == RB_BLACK &&
			    colour_of(m, rd_idx(&m->a.left, w)) == RB_BLACK) {
				if (w != NIL) {
					wr_colour(&m->a.colour, w, RB_RED);
				}
				x = x_parent;
				x_parent = (x_parent != NIL) ? rd_idx(&m->a.parent, x_parent) : NIL;
			} else {
				if (colour_of(m, rd_idx(&m->a.left, w)) == RB_BLACK) {
					size_t wr = rd_idx(&m->a.right, w);
					if (wr != NIL) {
						wr_colour(&m->a.colour, wr, RB_BLACK);
					}
					if (w != NIL) {
						wr_colour(&m->a.colour, w, RB_RED);
					}
					rotate_left(m, w);
					w = (x_parent != NIL) ? rd_idx(&m->a.left, x_parent) : NIL;
				}
				if (w != NIL) {
					wr_colour(&m->a.colour, w, rd_colour(&m->a.colour, x_parent));
				}
				if (x_parent != NIL) {
					wr_colour(&m->a.colour, x_parent, RB_BLACK);
				}
				size_t wl = (w != NIL) ? rd_idx(&m->a.left, w) : NIL;
				if (wl != NIL) {
					wr_colour(&m->a.colour, wl, RB_BLACK);
				}
				if (x_parent != NIL) {
					rotate_right(m, x_parent);
				}
				x = m->root;
				x_parent = NIL;
			}
		}
	}
	if (x != NIL) {
		wr_colour(&m->a.colour, x, RB_BLACK);
	}
}

static size_t find_index(const map *m, uintptr_t key) {
	size_t cur = m->root;
	while (cur != NIL) {
		int c = m->cmp(key, rd_up(&m->a.keys, cur), m->opaque);
		if (c == 0) {
			return cur;
		}
		if (c < 0) {
			cur = rd_idx(&m->a.left, cur);
		} else {
			cur = rd_idx(&m->a.right, cur);
		}
	}
	return NIL;
}

int map_insert(map *m, uintptr_t key, uintptr_t value) {
	size_t parent = NIL;
	size_t cur = m->root;

	while (cur != NIL) {
		int c = m->cmp(key, rd_up(&m->a.keys, cur), m->opaque);
		if (c == 0) {
			return 0; /* exists */
		}
		parent = cur;
		if (c < 0) {
			cur = rd_idx(&m->a.left, cur);
		} else {
			cur = rd_idx(&m->a.right, cur);
		}
	}

	size_t z;
	if (node_new(m, key, value, parent, &z) != 0) {
		return -1;
	}

	if (parent == NIL) {
		m->root = z;
	} else if (m->cmp(key, rd_up(&m->a.keys, parent), m->opaque) < 0) {
		wr_idx(&m->a.left, parent, z);
	} else {
		wr_idx(&m->a.right, parent, z);
	}

	insert_fixup(m, z);
	m->count += 1;
	return 1;
}

int map_insert_or_assign(map *m, uintptr_t key, uintptr_t value) {
	size_t parent = NIL;
	size_t cur = m->root;

	while (cur != NIL) {
		int c = m->cmp(key, rd_up(&m->a.keys, cur), m->opaque);
		if (c == 0) {
			wr_up(&m->a.vals, cur, value);
			return 0; /* updated */
		}
		parent = cur;
		if (c < 0) {
			cur = rd_idx(&m->a.left, cur);
		} else {
			cur = rd_idx(&m->a.right, cur);
		}
	}

	size_t z;
	if (node_new(m, key, value, parent, &z) != 0) {
		return -1;
	}

	if (parent == NIL) {
		m->root = z;
	} else if (m->cmp(key, rd_up(&m->a.keys, parent), m->opaque) < 0) {
		wr_idx(&m->a.left, parent, z);
	} else {
		wr_idx(&m->a.right, parent, z);
	}

	insert_fixup(m, z);
	m->count += 1;
	return 1; /* inserted */
}

int map_find(const map *m, uintptr_t key, uintptr_t *out_value) {
	size_t i = find_index(m, key);
	if (i == NIL) {
		return 0;
	}
	if (out_value != NULL) {
		*out_value = rd_up(&m->a.vals, i);
	}
	return 1;
}

uintptr_t *map_find_ref(map *m, uintptr_t key) {
	size_t i = find_index(m, key);
	if (i == NIL) {
		return NULL;
	}
	uintptr_t *vals = vector_data(&m->a.vals);
	return &vals[i];
}

int map_erase(map *m, uintptr_t key) {
	size_t z = find_index(m, key);
	if (z == NIL) {
		return 0;
	}

	size_t y = z;
	int y_colour = rd_colour(&m->a.colour, y);
	size_t x = NIL;
	size_t x_parent = NIL;

	if (rd_idx(&m->a.left, z) == NIL) {
		x = rd_idx(&m->a.right, z);
		x_parent = rd_idx(&m->a.parent, z);
		transplant(m, z, rd_idx(&m->a.right, z));
	} else if (rd_idx(&m->a.right, z) == NIL) {
		x = rd_idx(&m->a.left, z);
		x_parent = rd_idx(&m->a.parent, z);
		transplant(m, z, rd_idx(&m->a.left, z));
	} else {
		y = minimum_node(m, rd_idx(&m->a.right, z));
		y_colour = rd_colour(&m->a.colour, y);
		x = rd_idx(&m->a.right, y);
		if (rd_idx(&m->a.parent, y) == z) {
			x_parent = y;
		} else {
			x_parent = rd_idx(&m->a.parent, y);
			transplant(m, y, rd_idx(&m->a.right, y));
			wr_idx(&m->a.right, y, rd_idx(&m->a.right, z));
			wr_idx(&m->a.parent, rd_idx(&m->a.right, y), y);
		}
		transplant(m, z, y);
		wr_idx(&m->a.left, y, rd_idx(&m->a.left, z));
		wr_idx(&m->a.parent, rd_idx(&m->a.left, y), y);
		wr_colour(&m->a.colour, y, rd_colour(&m->a.colour, z));
	}

	node_recycle(m, z);
	m->count -= 1;

	if (y_colour == RB_BLACK) {
		erase_fixup(m, x, x_parent);
	}
	if (m->root != NIL) {
		wr_colour(&m->a.colour, m->root, RB_BLACK);
	}
	return 1;
}

int map_lower_bound(const map *m, uintptr_t key, map_iter *it_out) {
	if (it_out == NULL) {
		return 0;
	}
	size_t cur = m->root;
	size_t best = NIL;
	while (cur != NIL) {
		int c = m->cmp(rd_up(&m->a.keys, cur), key, m->opaque);
		if (c < 0) {
			cur = rd_idx(&m->a.right, cur);
		} else {
			best = cur;
			cur = rd_idx(&m->a.left, cur);
		}
	}
	it_out->owner = m;
	it_out->i = best;
	it_out->state = (best == NIL) ? ITER_END : ITER_OK;
	return best != NIL ? 1 : 0;
}

int map_upper_bound(const map *m, uintptr_t key, map_iter *it_out) {
	if (it_out == NULL) {
		return 0;
	}
	size_t cur = m->root;
	size_t best = NIL;
	while (cur != NIL) {
		int c = m->cmp(rd_up(&m->a.keys, cur), key, m->opaque);
		if (c <= 0) {
			cur = rd_idx(&m->a.right, cur);
		} else {
			best = cur;
			cur = rd_idx(&m->a.left, cur);
		}
	}
	it_out->owner = m;
	it_out->i = best;
	it_out->state = (best == NIL) ? ITER_END : ITER_OK;
	return best != NIL ? 1 : 0;
}

int map_begin(const map *m, map_iter *it_out) {
	if (it_out == NULL) {
		return 0;
	}
	it_out->owner = m;
	if (m->root == NIL) {
		it_out->i = NIL;
		it_out->state = ITER_END;
		return 0;
	}
	it_out->i = minimum_node(m, m->root);
	it_out->state = ITER_OK;
	return 1;
}

void map_end(const map *m, map_iter *it_out) {
	if (it_out == NULL) {
		return;
	}
	it_out->owner = m;
	it_out->i = NIL;
	it_out->state = ITER_END;
}

int map_next(map_iter *it) {
	if (it == NULL || it->owner == NULL) {
		return 0;
	}
	if (it->state == ITER_END) {
		return 0;
	}
	if (it->state == ITER_BEFORE_BEGIN) {
		if (it->owner->root == NIL) {
			it->state = ITER_END;
			it->i = NIL;
			return 0;
		}
		it->i = minimum_node(it->owner, it->owner->root);
		it->state = ITER_OK;
		return 1;
	}
	size_t nxt = successor(it->owner, it->i);
	if (nxt == NIL) {
		it->i = NIL;
		it->state = ITER_END;
		return 0;
	}
	it->i = nxt;
	it->state = ITER_OK;
	return 1;
}

int map_prev(map_iter *it) {
	if (it == NULL || it->owner == NULL) {
		return 0;
	}
	if (it->state == ITER_BEFORE_BEGIN) {
		return 0;
	}
	if (it->state == ITER_END) {
		if (it->owner->root == NIL) {
			return 0;
		}
		it->i = maximum_node(it->owner, it->owner->root);
		it->state = ITER_OK;
		return 1;
	}
	size_t prv = predecessor(it->owner, it->i);
	if (prv == NIL) {
		it->i = NIL;
		it->state = ITER_BEFORE_BEGIN;
		return 0;
	}
	it->i = prv;
	it->state = ITER_OK;
	return 1;
}

uintptr_t map_iter_key(const map_iter *it) {
	return rd_up(&it->owner->a.keys, it->i);
}

uintptr_t map_iter_value(const map_iter *it) {
	return rd_up(&it->owner->a.vals, it->i);
}

uintptr_t *map_iter_value_ref(map_iter *it) {
	uintptr_t *vals = vector_data(&it->owner->a.vals);
	return &vals[it->i];
}

int map_iter_equal(const map_iter *a, const map_iter *b) {
	if (a == NULL || b == NULL) {
		return 0;
	}
	if (a->owner != b->owner) {
		return 0;
	}
	if (a->state != b->state) {
		return 0;
	}
	if (a->state != ITER_OK) {
		return 1;
	}
	return a->i == b->i ? 1 : 0;
}

int map_cmp_u64(uintptr_t a, uintptr_t b, void *opaque) {
	(void)opaque;
	uint64_t ua = (uint64_t)a;
	uint64_t ub = (uint64_t)b;
	if (ua < ub) {
		return -1;
	}
	if (ua > ub) {
		return 1;
	}
	return 0;
}

int map_cmp_cstr(uintptr_t a, uintptr_t b, void *opaque) {
	const char *sa = (const char *)a;
	const char *sb = (const char *)b;
	if (sa == NULL && sb == NULL) {
		return 0;
	}
	if (sa == NULL) {
		return -1;
	}
	if (sb == NULL) {
		return 1;
	}
	return strcmp(sa, sb);
}

int map_cmp_cstr_nocase(uintptr_t a, uintptr_t b, void *opaque) {
	const char *sa = (const char *)a;
	const char *sb = (const char *)b;
	if (sa == NULL && sb == NULL) {
		return 0;
	}
	if (sa == NULL) {
		return -1;
	}
	if (sb == NULL) {
		return 1;
	}
	return strcasecmp(sa, sb);
}
