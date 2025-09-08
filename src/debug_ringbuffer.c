#include <kernel.h>

typedef struct {
	char   *buf;
	size_t  cap;    /* capacity in bytes */
	size_t  head;   /* index of oldest byte */
	size_t  tail;   /* index one past newest byte */
	bool    full;   /* true if buffer is full */
} dprintf_ring_t;

static dprintf_ring_t dprintf_ring = {0};

static inline size_t ring_used(const dprintf_ring_t *r) {
	if (r->full) {
		return r->cap;
	}
	if (r->tail >= r->head) {
		return r->tail - r->head;
	} else {
		return r->cap - (r->head - r->tail);
	}
}

size_t dprintf_size() {
	if (!dprintf_ring.buf || dprintf_ring.cap == 0) {
		return NULL;
	}

	return ring_used(&dprintf_ring);
}

static inline size_t ring_free(const dprintf_ring_t *r) {
	return r->cap - ring_used(r);
}

static inline size_t ring_advance(size_t idx, size_t n, size_t cap) {
	idx += n;
	if (idx >= cap) {
		idx -= cap;
	}
	return idx;
}

/* Evict exactly one line (up to and including '\n').
 * Returns true if a line was evicted, false if no complete line exists. */
static bool ring_drop_one_line(dprintf_ring_t *r) {
	if (ring_used(r) == 0) {
		return false;
	}

	size_t i = r->head;
	size_t used = ring_used(r);
	size_t scanned = 0;
	bool found_nl = false;

	while (scanned < used) {
		if (r->buf[i] == '\n') {
			found_nl = true;
			i = ring_advance(i, 1, r->cap); /* drop through newline */
			break;
		}
		i = ring_advance(i, 1, r->cap);
		scanned++;
	}

	if (!found_nl) {
		/* No newline in buffer: drop everything (degenerate very-long line case) */
		r->head = r->tail;
		r->full = false;
		return false;
	}

	r->head = i;
	r->full = false;
	return true;
}

/* Write arbitrary bytes into the ring (assumes enough space is available). */
void ring_write(dprintf_ring_t *r, const char *data, size_t len) {
	size_t first = r->cap - r->tail;
	if (first > len) {
		first = len;
	}
	memcpy(r->buf + r->tail, data, first);
	r->tail = ring_advance(r->tail, first, r->cap);

	size_t remain = len - first;
	if (remain > 0) {
		memcpy(r->buf + r->tail, data + first, remain);
		r->tail = ring_advance(r->tail, remain, r->cap);
	}

	if (r->tail == r->head) {
		r->full = true;
	}
}

bool dprintf_buffer_init(size_t cap_bytes) {
	if (cap_bytes == 0) {
		cap_bytes = 1u << 20; /* 1 MiB */
	}

	char *mem = (char *)kmalloc(cap_bytes);
	if (!mem) {
		return false;
	}

	if (dprintf_ring.buf) {
		kfree(dprintf_ring.buf);
	}

	dprintf_ring.buf  = mem;
	dprintf_ring.cap  = cap_bytes;
	dprintf_ring.head = 0;
	dprintf_ring.tail = 0;
	dprintf_ring.full = false;

	return true;
}

void dprintf_buffer_append_line(const char *line, size_t len) {
	if (!dprintf_ring.buf || dprintf_ring.cap == 0) {
		return;
	}

	bool needs_nl = true;
	if (len > 0) {
		if (line[len - 1] == '\n') {
			needs_nl = false;
		}
	}

	size_t needed = len + (needs_nl ? 1 : 0);

	/* If the line is larger than capacity, keep only the tail-end that fits. */
	if (needed > dprintf_ring.cap) {
		size_t keep = dprintf_ring.cap - 1;
		size_t drop = needed - 1 - keep;
		line += drop;
		len = keep;
		needs_nl = true;
		needed = len + 1;
	}

	while (ring_free(&dprintf_ring) < needed) {
		if (!ring_drop_one_line(&dprintf_ring)) {
			/* No complete line to drop: clear the buffer. */
			dprintf_ring.head = dprintf_ring.tail;
			dprintf_ring.full = false;
			break;
		}
	}

	if (len > 0) {
		ring_write(&dprintf_ring, line, len);
	}
	if (needs_nl) {
		char nl = '\n';
		ring_write(&dprintf_ring, &nl, 1);
	}
}

char *dprintf_buffer_snapshot(void) {
	if (!dprintf_ring.buf || dprintf_ring.cap == 0) {
		return NULL;
	}

	size_t used = ring_used(&dprintf_ring);

	/* +1 for null terminator */
	char *out = kmalloc(used + 1);
	memset(out, 0, used + 1);
	if (!out) {
		return NULL;
	}

	if (used == 0) {
		out[0] = '\0';
		return out;
	}

	if (dprintf_ring.head < dprintf_ring.tail || dprintf_ring.full) {
		/* Data is contiguous in the ring */
		size_t first_len = (dprintf_ring.full ? dprintf_ring.cap : dprintf_ring.tail - dprintf_ring.head);
		memcpy(out, dprintf_ring.buf + dprintf_ring.head, first_len);
	} else {
		/* Wrapped around */
		size_t first_len = dprintf_ring.cap - dprintf_ring.head;
		memcpy(out, dprintf_ring.buf + dprintf_ring.head, first_len);
		memcpy(out + first_len, dprintf_ring.buf, dprintf_ring.tail);
	}

	out[used] = '\0';
	return out;
}
