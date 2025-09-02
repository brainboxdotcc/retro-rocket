#include <kernel.h>

int vector_init(vector *v, size_t initial_capacity) {
	v->count = 0;
	v->capacity = 0;
	v->data = NULL;

	if (initial_capacity > 0) {
		v->data = (uintptr_t *)kmalloc(initial_capacity * sizeof(uintptr_t));
		if (v->data == NULL) {
			return -1;
		}
		v->capacity = initial_capacity;
	}
	return 0;
}

void vector_free(vector *v) {
	kfree_null(&v->data);
	v->count = 0;
	v->capacity = 0;
}

void vector_clear(vector *v) {
	v->count = 0;
}

size_t vector_grow_formula(size_t old_cap) {
	size_t bump = old_cap >> 1;
	size_t next = old_cap + bump;
	if (next < 8) {
		next = 8;
	}
	if (next < old_cap) {
		next = (size_t)-1 / sizeof(uintptr_t);
	}
	return next;
}

int vector_reserve(vector *v, size_t want_cap) {
	if (want_cap <= v->capacity) {
		return 0;
	}

	size_t new_cap = v->capacity;
	if (new_cap == 0) {
		new_cap = 8;
	}
	while (new_cap < want_cap) {
		size_t next = vector_grow_formula(new_cap);
		if (next <= new_cap) {
			new_cap = want_cap;
			break;
		}
		new_cap = next;
	}

	uintptr_t *p;
	if (v->data == NULL) {
		p = (uintptr_t *)kmalloc(new_cap * sizeof(uintptr_t));
	} else {
		p = (uintptr_t *)krealloc(v->data, new_cap * sizeof(uintptr_t));
	}

	if (p == NULL) {
		return -1;
	}

	v->data = p;
	v->capacity = new_cap;
	return 0;
}

size_t vector_size(const vector *v) {
	return v->count;
}

uintptr_t vector_get(const vector *v, size_t i) {
	return v->data[i];
}

void vector_set(vector *v, size_t i, uintptr_t value) {
	v->data[i] = value;
}

int vector_push(vector *v, uintptr_t value) {
	if (v->count == v->capacity) {
		if (vector_reserve(v, v->count + 1) != 0) {
			return -1;
		}
	}
	v->data[v->count] = value;
	v->count += 1;
	return 0;
}

uintptr_t vector_pop(vector *v) {
	v->count -= 1;
	return v->data[v->count];
}

int vector_insert_shift(vector *v, size_t i, uintptr_t value) {
	if (i > v->count) {
		return -1;
	}
	if (v->count == v->capacity) {
		if (vector_reserve(v, v->count + 1) != 0) {
			return -1;
		}
	}
	size_t tail = v->count - i;
	if (tail > 0) {
		memmove(v->data + i + 1, v->data + i, tail * sizeof(uintptr_t));
	}
	v->data[i] = value;
	v->count += 1;
	return 0;
}

int vector_erase_shift(vector *v, size_t i) {
	if (i >= v->count) {
		return -1;
	}
	size_t tail = v->count - i - 1;
	if (tail > 0) {
		memmove(v->data + i, v->data + i + 1, tail * sizeof(uintptr_t));
	}
	v->count -= 1;
	return 0;
}

int vector_erase_swap(vector *v, size_t i) {
	if (i >= v->count) {
		return -1;
	}
	size_t last = v->count - 1;
	v->data[i] = v->data[last];
	v->count = last;
	return 0;
}

int vector_insert(vector *v, size_t i, uintptr_t value) {
	return vector_insert_shift(v, i, value);
}

int vector_erase(vector *v, size_t i) {
	return vector_erase_shift(v, i);
}

uintptr_t *vector_data(vector *v) {
	return v->data;
}

const uintptr_t *vector_cdata(const vector *v) {
	return v->data;
}