#include <kernel.h>
#include <basic/memory_grants.h>

static inline bool is_red(memory_grant_t *n) {
	if (!n) {
		return false;
	}
	return n->red;
}

static void rotate_left(memory_grants_t *grants, memory_grant_t *x) {
	memory_grant_t *y = x->right;

	x->right = y->left;

	if (y->left) {
		y->left->parent = x;
	}

	y->parent = x->parent;

	if (!x->parent) {
		grants->root = y;
	} else if (x == x->parent->left) {
		x->parent->left = y;
	} else {
		x->parent->right = y;
	}

	y->left = x;
	x->parent = y;
}

static void rotate_right(memory_grants_t *grants, memory_grant_t *x) {
	memory_grant_t *y = x->left;

	x->left = y->right;

	if (y->right) {
		y->right->parent = x;
	}

	y->parent = x->parent;

	if (!x->parent) {
		grants->root = y;
	} else if (x == x->parent->right) {
		x->parent->right = y;
	} else {
		x->parent->left = y;
	}

	y->right = x;
	x->parent = y;
}

static void insert_fixup(memory_grants_t *grants, memory_grant_t *z) {
	while (is_red(z->parent)) {
		if (z->parent == z->parent->parent->left) {
			memory_grant_t *y = z->parent->parent->right;

			if (is_red(y)) {
				z->parent->red = false;
				y->red = false;
				z->parent->parent->red = true;
				z = z->parent->parent;
			} else {
				if (z == z->parent->right) {
					z = z->parent;
					rotate_left(grants, z);
				}

				z->parent->red = false;
				z->parent->parent->red = true;

				rotate_right(grants, z->parent->parent);
			}
		} else {
			memory_grant_t *y = z->parent->parent->left;

			if (is_red(y)) {
				z->parent->red = false;
				y->red = false;
				z->parent->parent->red = true;
				z = z->parent->parent;
			} else {
				if (z == z->parent->left) {
					z = z->parent;
					rotate_right(grants, z);
				}

				z->parent->red = false;
				z->parent->parent->red = true;

				rotate_left(grants, z->parent->parent);
			}
		}
	}

	grants->root->red = false;
}

void memory_grants_init(memory_grants_t *grants) {
	if (!grants) {
		return;
	}

	grants->root = NULL;
}

bool memory_grants_add(memory_grants_t *grants, buddy_allocator_t *allocator, void *base, size_t length) {
	if (!grants || !allocator || !base || length == 0) {
		return false;
	}

	memory_grant_t *node = buddy_calloc(allocator, 1, sizeof(memory_grant_t));

	if (!node) {
		return false;
	}

	node->base = base;
	node->end = node->base + length - 1;
	node->red = true;

	memory_grant_t *parent = NULL;
	memory_grant_t **link = &grants->root;

	while (*link) {
		parent = *link;

		if (node->base < parent->base) {
			link = &parent->left;
		} else if (node->base > parent->base) {
			link = &parent->right;
		} else {
			buddy_free(allocator, node);
			return false;
		}
	}

	node->parent = parent;
	*link = node;

	insert_fixup(grants, node);

	return true;
}

bool memory_grants_contains(memory_grants_t *grants, void *addr, size_t length) {
	if (!grants || !addr || length == 0) {
		return false;
	}

	uint8_t *start = addr;
	uint8_t *end = start + length - 1;

	memory_grant_t *node = grants->root;

	while (node) {
		if (start < node->base) {
			node = node->left;
			continue;
		}

		if (start > node->end) {
			node = node->right;
			continue;
		}

		return end <= node->end;
	}

	return false;
}

static memory_grant_t *tree_minimum(memory_grant_t *node) {
	while (node && node->left) {
		node = node->left;
	}

	return node;
}

static void transplant(memory_grants_t *grants, memory_grant_t *u, memory_grant_t *v) {
	if (!u->parent) {
		grants->root = v;
	} else if (u == u->parent->left) {
		u->parent->left = v;
	} else {
		u->parent->right = v;
	}

	if (v) {
		v->parent = u->parent;
	}
}

static void delete_fixup(memory_grants_t *grants, memory_grant_t *x, memory_grant_t *x_parent) {
	while (x_parent && (x != grants->root) && !is_red(x)) {
		if (x == x_parent->left) {
			memory_grant_t *w = x_parent->right;

			if (is_red(w)) {
				w->red = false;
				x_parent->red = true;
				rotate_left(grants, x_parent);
				w = x_parent->right;
			}

			if (!w || ((!w->left || !is_red(w->left)) && (!w->right || !is_red(w->right)))) {
				if (w) {
					w->red = true;
				}

				x = x_parent;
				x_parent = x ? x->parent : NULL;
			} else {
				if (!w->right || !is_red(w->right)) {
					if (w->left) {
						w->left->red = false;
					}

					w->red = true;
					rotate_right(grants, w);
					w = x_parent->right;
				}

				w->red = x_parent->red;
				x_parent->red = false;

				if (w->right) {
					w->right->red = false;
				}

				rotate_left(grants, x_parent);
				x = grants->root;
				break;
			}
		} else {
			memory_grant_t *w = x_parent->left;

			if (is_red(w)) {
				w->red = false;
				x_parent->red = true;
				rotate_right(grants, x_parent);
				w = x_parent->left;
			}

			if (!w || ((!w->right || !is_red(w->right)) && (!w->left || !is_red(w->left)))) {
				if (w) {
					w->red = true;
				}

				x = x_parent;
				x_parent = x ? x->parent : NULL;
			} else {
				if (!w->left || !is_red(w->left)) {
					if (w->right) {
						w->right->red = false;
					}

					w->red = true;
					rotate_left(grants, w);
					w = x_parent->left;
				}

				w->red = x_parent->red;
				x_parent->red = false;

				if (w->left) {
					w->left->red = false;
				}

				rotate_right(grants, x_parent);
				x = grants->root;
				break;
			}
		}
	}

	if (x) {
		x->red = false;
	}
}

bool memory_grants_remove(memory_grants_t *grants, buddy_allocator_t *allocator, void *base) {
	if (!grants || !allocator || !base) {
		return false;
	}

	memory_grant_t *z = grants->root;

	while (z) {
		if ((uint8_t *)base < z->base) {
			z = z->left;
		} else if ((uint8_t *)base > z->base) {
			z = z->right;
		} else {
			break;
		}
	}

	if (!z) {
		return false;
	}

	memory_grant_t *y = z;
	memory_grant_t *x = NULL;
	memory_grant_t *x_parent = NULL;

	bool y_original_red = y->red;

	if (!z->left) {
		x = z->right;
		x_parent = z->parent;

		transplant(grants, z, z->right);
	} else if (!z->right) {
		x = z->left;
		x_parent = z->parent;

		transplant(grants, z, z->left);
	} else {
		y = tree_minimum(z->right);

		y_original_red = y->red;

		x = y->right;

		if (y->parent == z) {
			x_parent = y;
		} else {
			x_parent = y->parent;

			transplant(grants, y, y->right);

			y->right = z->right;
			y->right->parent = y;
		}

		transplant(grants, z, y);

		y->left = z->left;
		y->left->parent = y;

		y->red = z->red;
	}

	if (!y_original_red) {
		delete_fixup(grants, x, x_parent);
	}

	buddy_free(allocator, z);

	return true;
}

bool memory_grants_valid_base(memory_grants_t *grants, void *base) {
	if (!grants || !base) {
		return false;
	}
	uint8_t *addr = base;
	memory_grant_t *node = grants->root;
	while (node) {
		if (addr < node->base) {
			node = node->left;
		} else if (addr > node->base) {
			node = node->right;
		} else {
			return true;
		}
	}
	return false;
}
