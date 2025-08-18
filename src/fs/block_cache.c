#include <kernel.h>

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
 * sector buffer, recency information, and heap position. Entries are stored
 * in the cache's hashmap and referenced from the min-heap.
 */
typedef struct block_cache_entry {
	uint64_t lba;          /**< Absolute sector number on the device. */
	uint8_t *buf;          /**< Pointer to sector-sized buffer containing data. */
	uint64_t last_used;    /**< Monotonic tick value; updated on every access. */
	uint32_t heap_index;   /**< Current index in the heap array. */
} block_cache_entry_t;

/**
 * @brief Per-device block cache.
 *
 * A cache instance attached to a storage device. Holds a bounded number of
 * sector entries in a hashmap for O(1) lookup and in a min-heap for O(log N)
 * eviction. Tracks recency using a monotonically increasing tick counter.
 * All access is synchronised by a spinlock.
 */
typedef struct block_cache {
	storage_device_t *dev;         /**< Backing storage device. */
	uint32_t sector_size;          /**< Sector size in bytes. */
	uint32_t cap;                  /**< Maximum cache size in sectors. */
	struct hashmap *map;           /**< Maps LBA -> entry pointer. */
	block_cache_entry_t **heap;    /**< Min-heap ordered by last_used. */
	uint32_t heap_size;            /**< Current number of entries in the heap. */
	uint64_t tick;                 /**< Monotonic counter for recency stamps. */
	spinlock_t lock;               /**< Spinlock protecting cache state. */
} block_cache_t;

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

/**
 * @brief Swap two heap positions and fix their @ref heap_index fields.
 *
 * Internal helper for heap maintenance.
 *
 * @param c  Cache instance.
 * @param i  First index.
 * @param j  Second index.
 */
static inline void heap_swap(block_cache_t *c, uint32_t i, uint32_t j) {
	block_cache_entry_t *ei = c->heap[i];
	block_cache_entry_t *ej = c->heap[j];
	c->heap[i] = ej;
	c->heap[j] = ei;
	ej->heap_index = i;
	ei->heap_index = j;
}

/**
 * @brief Restore heap order upwards from a node.
 *
 * Bubbles the node at index @p i towards the root while its key is smaller
 * than its parent.
 *
 * @param c  Cache instance.
 * @param i  Start index to heapify from.
 */
static void heapify_up(block_cache_t *c, uint32_t i) {
	while (i > 0) {
		uint32_t p = (i - 1) / 2;
		if (c->heap[p]->last_used <= c->heap[i]->last_used) {
			break;
		}
		heap_swap(c, p, i);
		i = p;
	}
}

/**
 * @brief Restore heap order downwards from a node.
 *
 * Sifts the node at index @p i down until both children are not smaller.
 *
 * @param c  Cache instance.
 * @param i  Start index to heapify from.
 */
static void heapify_down(block_cache_t *c, uint32_t i) {
	for (;;) {
		uint32_t left = i * 2 + 1;
		uint32_t right = i * 2 + 2;
		uint32_t smallest = i;

		if (left < c->heap_size && c->heap[left]->last_used < c->heap[smallest]->last_used) {
			smallest = left;
		}
		if (right < c->heap_size && c->heap[right]->last_used < c->heap[smallest]->last_used) {
			smallest = right;
		}
		if (smallest == i) {
			break;
		}
		heap_swap(c, i, smallest);
		i = smallest;
	}
}

/**
 * @brief Insert an entry pointer into the min-heap.
 *
 * Appends at the end and restores heap order.
 *
 * @param c  Cache instance.
 * @param e  Entry to push (already part of the hashmap).
 */
static void heap_push(block_cache_t *c, block_cache_entry_t *e) {
	uint32_t i = c->heap_size;
	c->heap[i] = e;
	e->heap_index = i;
	c->heap_size++;
	heapify_up(c, i);
}

/**
 * @brief Remove and return the root (oldest) entry from the heap.
 *
 * @param c  Cache instance.
 * @return   Pointer to the oldest entry, or NULL if the heap is empty.
 */
static block_cache_entry_t *heap_pop_root(block_cache_t *c) {
	if (c->heap_size == 0) {
		return NULL;
	}
	block_cache_entry_t *root = c->heap[0];
	uint32_t last = c->heap_size - 1;
	c->heap_size--;
	if (c->heap_size > 0) {
		c->heap[0] = c->heap[last];
		c->heap[0]->heap_index = 0;
		heapify_down(c, 0);
	}
	return root;
}

/**
 * @brief Reposition a node after its @ref last_used increased.
 *
 * Since the key only increases on access, the node can only move down.
 *
 * @param c  Cache instance.
 * @param e  Entry whose key increased.
 */
static void heap_adjust_after_increase(block_cache_t *c, block_cache_entry_t *e) {
	heapify_down(c, e->heap_index);
}

/**
 * @brief Evict the globally oldest entry from the cache.
 *
 * Pops the heap root, removes the entry from the hashmap, and frees its
 * sector buffer. No-op if the heap is empty.
 *
 * @param c  Cache instance.
 */
static void evict_one(block_cache_t *c) {
	block_cache_entry_t *oldest = heap_pop_root(c);
	if (!oldest) {
		return;
	}

	/* Free sub-resources before removing map item */
	if (oldest->buf) {
		kfree_null(&oldest->buf);
	}

	block_cache_entry_t key = { .lba = oldest->lba };
	hashmap_delete(c->map, &key);
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
	c->heap_size = 0;
	c->tick = 0;
	init_spinlock(&c->lock);

	c->map = hashmap_new(sizeof(block_cache_entry_t), 0, CACHE_HASH_SEED0, CACHE_HASH_SEED1, cache_hash, cache_compare, NULL, NULL);
	if (!c->map) {
		kfree_null(&c);
		return NULL;
	}

	c->heap = kmalloc(sizeof(block_cache_entry_t *) * c->cap);
	if (!c->heap) {
		hashmap_free(c->map);
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

	while (c->heap_size > 0) {
		evict_one(c);
	}

	unlock_spinlock_irq(&c->lock, flags);

	if (c->heap) {
		kfree_null(&c->heap);
	}
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

	uint32_t nsec = sectors_for_len(c->sector_size, bytes);
	if (lba + nsec > c->dev->size) {
		fs_set_error(FS_ERR_OUT_OF_BOUNDS);
		return 0;
	}

	for (uint32_t i = 0; i < nsec; i++) {
		uint64_t sector_lba = lba + i;
		unsigned char *dst = out + i * c->sector_size;

		uint64_t flags;
		lock_spinlock_irq(&c->lock, &flags);

		block_cache_entry_t find = { .lba = sector_lba };
		block_cache_entry_t *e = hashmap_get(c->map, &find);

		if (e) {
			e->last_used = c->tick + 1;
			c->tick = e->last_used;
			heap_adjust_after_increase(c, e);
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

	for (uint32_t i = 0; i < nsec; i++) {
		uint64_t sector_lba = lba + i;
		const unsigned char *seg = src + i * c->sector_size;

		uint64_t flags;
		lock_spinlock_irq(&c->lock, &flags);

		block_cache_entry_t find = { .lba = sector_lba };
		block_cache_entry_t *e = hashmap_get(c->map, &find);

		if (e) {
			memcpy(e->buf, seg, c->sector_size);
			e->last_used = c->tick + 1;
			c->tick = e->last_used;
			heap_adjust_after_increase(c, e);
			unlock_spinlock_irq(&c->lock, flags);
			continue;
		}

		if (c->heap_size == c->cap) {
			evict_one(c);
		}

		block_cache_entry_t temp = {0};
		temp.lba = sector_lba;
		temp.buf = kmalloc(c->sector_size);
		if (!temp.buf) {
			unlock_spinlock_irq(&c->lock, flags);
			fs_set_error(FS_ERR_OUT_OF_MEMORY);
			return 0;
		}

		memcpy(temp.buf, seg, c->sector_size);
		temp.last_used = c->tick + 1;
		c->tick = temp.last_used;

		hashmap_set(c->map, &temp);

		/* Get the stable, hashmap-owned pointer and push into the heap */
		block_cache_entry_t *stored = hashmap_get(c->map, &find);
		if (!stored) {
			/* Defensive clean-up if set failed unexpectedly */
			kfree_null(&temp.buf);
			unlock_spinlock_irq(&c->lock, flags);
			fs_set_error(FS_ERR_INTERNAL);
			return 0;
		}

		stored->last_used = temp.last_used;
		heap_push(c, stored);

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
	while (c->heap_size > 0) {
		evict_one(c);
	}
	unlock_spinlock_irq(&c->lock, flags);
}
