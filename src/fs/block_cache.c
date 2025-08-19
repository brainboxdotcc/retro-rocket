#include <kernel.h>
#include <buddy_allocator.h>

#ifndef BLOCK_CACHE_SECTOR_CAP
/**
 * @brief Global per-device cache capacity in sectors.
 *
 * This limit is applied to every cache instance. Increase with care; it
 * impacts memory consumption linearly (one sector buffer per entry).
 */
#define BLOCK_CACHE_SECTOR_CAP 8192
#endif

/* Hash seeds (arbitrary constants) */
#define CACHE_HASH_SEED0  1469598103934665603ULL
#define CACHE_HASH_SEED1  1099511628211ULL

/**
 * @brief A single cached sector entry.
 *
 * Represents one device sector held in the cache. Each entry tracks its LBA,
 * sector buffer, and recency information. Entries are stored in the cache's
 * hashmap. The LRU queue stores LBAs only to avoid stale pointers.
 */
typedef struct block_cache_entry {
	uint64_t lba;          /**< Absolute sector number on the device. */
	uint8_t *buf;          /**< Pointer to sector-sized buffer containing data. */
	uint64_t last_used;    /**< Monotonic tick value; updated on every access. */
} block_cache_entry_t;

/* LRU node carries only the LBA (no pointer to cache entry). */
typedef struct lru_node {
	uint64_t lba;
	struct lru_node *prev;
	struct lru_node *next;
} lru_node_t;

/**
 * @brief Per-device block cache.
 *
 * A cache instance attached to a storage device. Holds a bounded number of
 * sector entries in a hashmap for O(1) lookup and in an LRU queue for O(1)
 * eviction. Tracks recency using a monotonically increasing tick counter.
 * All access is synchronised by a spinlock.
 */
typedef struct block_cache {
	storage_device_t *dev;         /**< Backing storage device. */
	uint32_t sector_size;          /**< Sector size in bytes. */
	uint32_t cap;                  /**< Maximum cache size in sectors. */
	struct hashmap *map;           /**< Maps LBA -> entry pointer. */

	/* LRU queue (nodes store only LBA) */
	lru_node_t *lru_head;          /**< Oldest entry (eviction candidate). */
	lru_node_t *lru_tail;          /**< Most recently used entry. */
	uint32_t    lru_count;         /**< Current number of nodes in the LRU list. */

	uint64_t tick;                 /**< Monotonic counter for recency stamps. */
	spinlock_t lock;               /**< Spinlock protecting cache state. */
} block_cache_t;

static buddy_allocator_t cache_allocator = {};   /**< Buddy allocator for content */

/**
 * @brief Compare two cache entries by LBA (hashmap comparator).
 *
 * Expects pointers to @ref block_cache_entry_t. Returns negative, zero, or
 * positive according to standard qsort-style ordering.
 *
 * @param a      First entry.
 * @param b      Second entry.
 * @param udata  Unused user data pointer.
 * @return       <0 if @p a < @p b, 0 if equal, >0 if @p a > @p b.
 */
static int cache_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
const block_cache_entry_t *ea = a;
const block_cache_entry_t *eb = b;
if (ea->lba < eb->lba) {
return -1;
}
if (ea->lba > eb->lba) {
return 1;
}
return 0;
}

/**
 * @brief Hash a cache entry by LBA (hashmap hasher).
 *
 * Uses SipHash over the 64-bit LBA stored in the entry.
 *
 * @param item   Entry to hash (points to @ref block_cache_entry_t).
 * @param seed0  Primary seed.
 * @param seed1  Secondary seed.
 * @return       64-bit hash value.
 */
static uint64_t cache_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const block_cache_entry_t *e = item;
	return hashmap_sip(&e->lba, sizeof(e->lba), seed0, seed1);
}

/* ---------------- LRU helpers (nodes store only LBA) ---------------- */

static inline lru_node_t *lru_alloc_node(uint64_t lba) {
	lru_node_t *n = buddy_malloc(&cache_allocator, sizeof(lru_node_t));
	if (!n) {
		return NULL;
	}
	n->lba = lba;
	n->prev = n->next = NULL;
	return n;
}

static inline void lru_free_node(lru_node_t *n) {
	buddy_free(&cache_allocator, n);
}

/* Find first node with given LBA (O(n)). */
static lru_node_t *lru_find(block_cache_t *c, uint64_t lba) {
	for (lru_node_t *p = c->lru_head; p; p = p->next) {
		if (p->lba == lba) {
			return p;
		}
	}
	return NULL;
}

/* Unlink a node from the LRU list (does not free). */
static void lru_unlink(block_cache_t *c, lru_node_t *n) {
	if (!n) {
		return;
	}
	if (n->prev) {
		n->prev->next = n->next;
	} else {
		c->lru_head = n->next;
	}
	if (n->next) {
		n->next->prev = n->prev;
	} else {
		c->lru_tail = n->prev;
	}
	n->prev = n->next = NULL;
	c->lru_count--;
}

/* Push a node to the LRU tail (MRU). Node must be detached. */
static void lru_push_tail(block_cache_t *c, lru_node_t *n) {
	if (!n) {
		return;
	}
	n->prev = c->lru_tail;
	n->next = NULL;
	if (c->lru_tail) {
		c->lru_tail->next = n;
	}
	c->lru_tail = n;
	if (!c->lru_head) {
		c->lru_head = n;
	}
	c->lru_count++;
}

/* Move existing LBA to tail; if not present, create a node and append.
 * LAZY variant: never search; always append a fresh node (duplicates allowed).
 * Returns 1 on success, 0 on OOM.
 */
static int lru_touch(block_cache_t *c, uint64_t lba) {
	lru_node_t *n = buddy_malloc(&cache_allocator, sizeof(lru_node_t));
	if (!n) {
		return 0;
	}
	n->lba = lba;
	n->prev = c->lru_tail;
	n->next = NULL;
	if (c->lru_tail) {
		c->lru_tail->next = n;
	}
	c->lru_tail = n;
	if (!c->lru_head) {
		c->lru_head = n;
	}
	c->lru_count++;
	return 1;
}


/* Pop the LRU head (returns node; caller frees). */
static lru_node_t *lru_pop_head(block_cache_t *c) {
	lru_node_t *n = c->lru_head;
	if (!n) {
		return NULL;
	}
	lru_unlink(c, n); /* detaches and adjusts count */
	return n;
}

/**
 * @brief Evict the globally oldest entry from the cache.
 *
 * Pops the LRU head, removes the entry from the hashmap if present, and frees
 * its sector buffer. No-op if the LRU is empty.
 *
 * @param c  Cache instance.
 */
static void evict_one(block_cache_t *c)
{
	for (;;) {
		lru_node_t *n = lru_pop_head(c);
		if (!n) {
			return; /* nothing to evict */
		}
		uint64_t lba = n->lba;
		lru_free_node(n);

		block_cache_entry_t key = { .lba = lba };
		block_cache_entry_t *e = hashmap_get(c->map, &key);
		if (!e) {
			/* stale node; keep scanning */
			continue;
		}

		/* live entry: evict it */
		if (e->buf) {
			buddy_free(&cache_allocator, e->buf);
			e->buf = NULL;
		}
		hashmap_delete(c->map, &key);
		return;
	}
}

void* cache_malloc(size_t size) {
	void* p = buddy_malloc(&cache_allocator, size);
	return p;
}

void* cache_realloc(void* ptr, size_t size) {
	return buddy_realloc(&cache_allocator, ptr, size);
}

void cache_free(const void* ptr) {
	buddy_free(&cache_allocator, ptr);
}

block_cache_t *block_cache_create(storage_device_t *dev) {
	if (!dev) {
		return NULL;
	}
	block_cache_t *c = kmalloc(sizeof(block_cache_t));
	if (!c) {
		return NULL;
	}

	c->dev = dev;
	c->sector_size = dev->block_size;
	c->cap = BLOCK_CACHE_SECTOR_CAP;
	c->lru_head = NULL;
	c->lru_tail = NULL;
	c->lru_count = 0;
	c->tick = 0;
	init_spinlock(&c->lock);

	if (!cache_allocator.regions) {
		buddy_init(&cache_allocator, 6, 22, 22);
	}

	c->map = hashmap_new_with_allocator(cache_malloc, cache_realloc, cache_free, sizeof(block_cache_entry_t), 0, CACHE_HASH_SEED0, CACHE_HASH_SEED1, cache_hash, cache_compare, NULL, c);
	if (!c->map) {
		kfree_null(&c);
		return NULL;
	}

	return c;
}

void block_cache_destroy(block_cache_t **pcache) {
	if (!pcache || !*pcache) {
		return;
	}
	block_cache_t *c = *pcache;

	uint64_t flags;
	lock_spinlock_irq(&c->lock, &flags);

	while (c->lru_count > 0) {
		evict_one(c);
	}

	unlock_spinlock_irq(&c->lock, flags);

	if (c->map) {
		hashmap_free(c->map);
		c->map = NULL;
	}

	kfree_null(&c);
	*pcache = NULL;
}

/**
 * @brief Compute a sector count that covers a byte length.
 *
 * Rounds up @p bytes to a whole number of @p sector_size units. Used to
 * iterate multi-sector operations at sector granularity.
 *
 * @param sector_size  Device sector size in bytes.
 * @param bytes        Requested byte length.
 * @return             Number of sectors covering @p bytes (at least 1).
 */
static uint32_t sectors_for_len(uint32_t sector_size, uint32_t bytes) {
	uint32_t n = bytes / sector_size;
	while (n * sector_size < bytes) {
		n++;
	}
	if (n < 1) {
		n++;
	}
	return n;
}

int block_cache_read(block_cache_t *c, uint64_t lba, uint32_t bytes, unsigned char *out)
{
	if (!c || !c->dev || !out) {
		return 0;
	}

	uint64_t flags;

	uint32_t nsec = sectors_for_len(c->sector_size, bytes);
	if (lba + nsec > c->dev->size) {
		fs_set_error(FS_ERR_OUT_OF_BOUNDS);
		return 0;
	}

	for (uint32_t i = 0; i < nsec; i++) {
		uint64_t sector_lba = lba + i;
		unsigned char *dst = out + i * c->sector_size;

		lock_spinlock_irq(&c->lock, &flags);

		block_cache_entry_t find = { .lba = sector_lba };
		block_cache_entry_t *e = hashmap_get(c->map, &find);

		if (e) {
			e->last_used = c->tick + 1;
			c->tick = e->last_used;

			/* Move this LBA to MRU; if missing from LRU due to prior drift, append it. */
			(void)lru_touch(c, sector_lba);

			memcpy(dst, e->buf, c->sector_size);
			unlock_spinlock_irq(&c->lock, flags);
			continue;
		}

		unlock_spinlock_irq(&c->lock, flags);
		return 0;
	}

	return 1;
}

int block_cache_write(block_cache_t *c, uint64_t lba, uint32_t bytes, const unsigned char *src)
{
	if (!c || !c->dev || !src) {
		return 0;
	}

	uint32_t nsec = sectors_for_len(c->sector_size, bytes);
	if (lba + nsec > c->dev->size) {
		fs_set_error(FS_ERR_OUT_OF_BOUNDS);
		return 0;
	}

	uint64_t flags;

	for (uint32_t i = 0; i < nsec; i++) {
		uint64_t sector_lba = lba + i;
		const unsigned char *seg = src + i * c->sector_size;

		lock_spinlock_irq(&c->lock, &flags);

		block_cache_entry_t find = { .lba = sector_lba };
		block_cache_entry_t *e = hashmap_get(c->map, &find);

		if (e) {
			memcpy(e->buf, seg, c->sector_size);
			e->last_used = c->tick + 1;
			c->tick = e->last_used;

			/* Touch MRU position for this LBA. */
			lru_touch(c, sector_lba);

			unlock_spinlock_irq(&c->lock, flags);
			continue;
		}

		/* Ensure capacity before inserting a new entry. */
		while (c->lru_count >= c->cap) {
			evict_one(c);
		}

		block_cache_entry_t temp = {0};
		temp.lba = sector_lba;
		temp.buf = buddy_malloc(&cache_allocator, c->sector_size);
		if (!temp.buf) {
			unlock_spinlock_irq(&c->lock, flags);
			return 0;
		}

		memcpy(temp.buf, seg, c->sector_size);
		temp.last_used = c->tick + 1;
		c->tick = temp.last_used;

		hashmap_set(c->map, &temp);

		/* Get the stable, hashmap-owned pointer to confirm insert. */
		block_cache_entry_t *stored = hashmap_get(c->map, &find);
		if (!stored) {
			dprintf("BUG: hashed sector not stored in cache\n");
			buddy_free(&cache_allocator, temp.buf);
			unlock_spinlock_irq(&c->lock, flags);
			fs_set_error(FS_ERR_INTERNAL);
			return 0;
		}

		stored->last_used = temp.last_used;

		/* Place LBA at MRU; if node is somehow already present, it'll be moved. */
		if (!lru_touch(c, sector_lba)) {
			/* Out of memory creating the LRU node; roll back map insert. */
			block_cache_entry_t key = { .lba = sector_lba };
			(void)hashmap_delete(c->map, &key);
			buddy_free(&cache_allocator, stored->buf);
			unlock_spinlock_irq(&c->lock, flags);
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return 0;
		}

		unlock_spinlock_irq(&c->lock, flags);
	}

	return 1;
}

void block_cache_invalidate(block_cache_t *c) {
	if (!c) {
		return;
	}
	uint64_t flags;
	lock_spinlock_irq(&c->lock, &flags);
	while (c->lru_count > 0) {
		evict_one(c);
	}
	unlock_spinlock_irq(&c->lock, flags);
}
