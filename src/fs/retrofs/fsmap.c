#include <kernel.h>
#include <retrofs.h>

/* RetroFS free space map & hierarchical caches (L0/L1/L2)
 * =======================================================
 *
 * TL;DR
 * -----
 * - On disk we keep a Level-0 (L0) bitmap: 1 bit per sector (512 B).
 *   Bit 1 = used, 0 = free.
 * - At mount we stream L0 once and build tiny in-RAM summaries:
 *   L1 per-group counters/bitsets and L2 super-group bitsets.
 * - All allocation/free decisions are made using L1/L2 to minimise I/O.
 *   The L0 bitmap is always updated (write-through source of truth).
 *
 * Terminology
 * -----------
 * - Sector: 512 bytes; the allocation atom.
 * - L0 (bitmap): On-disk free space map, 1 bit/sector, 1=used, 0=free.
 * - Group (L1 unit): A contiguous block of sectors summarised as one L1 entry.
 *   We use `RFS_L1_GROUP_SECTORS == 4096` (2MB @ 512 B/sector).
 * - Super-group (L2 unit): A block of L1 groups summarised as one L2 bit.
 *   We use `RFS_L2_GROUPS_PER_SUPER == 1024` (i.e. 1024 L1 groups).
 *
 * On-disk layout (L0)
 * -------------------
 * - The description block (`rfs_description_block_t`) records:
 *     - `free_space_map_start`: LBA of the first L0 map sector (relative to the partition start).
 *     - `free_space_map_length`: L0 map length in sectors.
 * - Each L0 sector stores 4096 bits (512 B × 8), i.e. 4096 sectors worth of state.
 * - The map may be padded to a sector boundary. We always cap processing to the
 *   true number of allocatable sectors (`info->total_sectors`) and ignore padded tail bits.
 *
 * In-RAM caches built at mount
 * ----------------------------
 * We build a hierarchy that lets us skip vast unused/used regions without reading L0:
 *
 *  L1 (per-group; group size = G = RFS_L1_GROUP_SECTORS):
 *   - `uint16_t *l1_free_count`  [size = l1_groups] :
 *       Number of free sectors in each group, 0..G (tail group may be < G).
 *   - `uint8_t  *l1_not_full`    (bitset, 1 if `free_count > 0`).
 *   - `uint8_t  *l1_all_free`    (bitset, 1 if `free_count == group_size`).
 *
 *  L2 (per super-group; S = RFS_L2_GROUPS_PER_SUPER):
 *   - `uint8_t  *l2_not_full`    (bitset, 1 if any child L1 group has free space).
 *   - `uint8_t  *l2_all_free`    (bitset, 1 if *every* child L1 group is wholly free).
 *
 *  Sizes (order of magnitude):
 *   - For 1 TiB @ 512 B sectors: L0 ≈ 256 MB on disk; L1/L2 RAM ≈ 1.2 MB.
 *   - For small/retro disks it’s tiny.
 *
 * Mount-time build algorithm (fast, sequential)
 * --------------------------------------------
 * - Read the L0 bitmap sequentially in large chunks (`RFS_MAP_READ_CHUNK_SECTORS`, e.g. 128).
 * - For each chunk, process 64-bit words and use POPCNT (`-mpopcnt`) on the inverted words
 *   so “free” bits are 1s. We then add those counts into the appropriate L1 group(s),
 *   carefully splitting at group boundaries.
 * - After the single pass:
 *     - Derive `l1_not_full` and `l1_all_free` from `l1_free_count`.
 *     - Fold L1 into L2 by OR/AND across each super-group’s children.
 * - This pass touches L0 once and issues ~`map_length / chunk_size` I/Os (e.g. 4 reads for 495 sectors at 128-sector bursts).
 *
 * Allocation strategy (find-only)
 * -------------------------------
 * `rfs_find_free_extent(info, need_sectors, &start_out)`:
 * - If `need <= G` (fits in one group):
 *     1) Scan `l2_not_full` for a super-group with any free space.
 *     2) Within it, prefer groups marked `l1_all_free` (zero L0 I/O).
 *     3) Otherwise read that group’s single L0 sector and bit-scan for a run.
 * - If `need > G` (spans multiple groups):
 *     1) Pick a group with a free tail (either `l1_all_free` -> `tail=group_size`
 *        or a mixed group -> read L0 once and compute suffix-free).
 *     2) Chain across consecutive `l1_all_free` groups (no L0 I/O).
 *     3) If necessary, finish in the next group’s free prefix (read L0 once).
 * - This approach does at most two L0 sector reads per candidate chain (start/end),
 *   and none when entirely covered by all-free groups.
 * - The function only finds a location; it does not modify the map.
 *
 * Allocation / free (commit)
 * --------------------------
 * `rfs_mark_extent(info, start, length, mark_used)` performs the write-through update:
 * - Whole-group fast path (extent exactly covers one or more full groups):
 *     - Write the corresponding L0 sector(s) as all `0xFF` (reserve) or `0x00` (free)
 *       - no read required.
 *     - Update `l1_free_count[g]` to 0 or `group_size`, and set:
 *         - `l1_not_full[g] = !mark_used`
 *         - `l1_all_free[g] = !mark_used` (only true when freeing an entire group)
 *     - Recompute the affected L2 super-group bits by folding its children
 *       (cheap; S is modest).
 * - Partial-group path (edge groups):
 *     - Read the group’s single L0 sector.
 *     - OR (reserve) or AND (free) the exact bit range.
 *     - Write it back.
 *     - Adjust `l1_free_count[g]` by the exact number of 0->1 or 1->0 transitions,
 *       then refresh `l1_not_full[g]` and `l1_all_free[g]`.
 *     - Refresh the corresponding L2 super-group bits.
 * - Important invariants:
 *     - Update L1/L2 only after a successful L0 write (RAM mirrors disk).
 *     - All bounds are in sectors; I/O sizes are sector-aligned bytes.
 *     - The bitmap itself and all fixed metadata (superblock, root dir, etc.)
 *       are marked used at format time, so allocators never hand them out.
 *
 * Why the hierarchy matters
 * -------------------------
 * - The L0 bitmap for large volumes is too big to scan linearly for every allocation.
 *   L1/L2 let us skip hundreds of thousands of sectors in a single test.
 * - `l1_all_free` is the secret sauce for big contiguous allocations: we can stitch
 *   together long runs without reading L0 at all, and only touch the two edge groups.
 * - `l1_free_count` ensures O(1) updates on allocate/free (no rescans needed to
 *   detect when a group became full or fully free).
 *
 * Tunables
 * --------
 * - `RFS_L1_GROUP_SECTORS` (G):
 *     - Larger G -> smaller L1, fewer updates, better big-run performance,
 *       but coarser granularity for “fits in one group”.
 *     - We default to 4096 sectors (2 MB) so each group maps 1:1 to a single L0 sector.
 * - `RFS_L2_GROUPS_PER_SUPER` (S):
 *     - Larger S -> smaller L2, fewer L2 updates, but slightly slower skipping if very sparse.
 *     - We default to 1024, which keeps L2 tiny.
 * - `RFS_MAP_READ_CHUNK_SECTORS`:
 *     - Larger chunk size reduces IRQ/I/O overhead when building caches at mount.
 *       128 sectors (64 KiB) is a sweet spot for our AHCI path; the driver can go higher later.
 *
 * Performance notes
 * -----------------
 * - Mount-time L0 scan cost is essentially sequential bandwidth bound.
 *   SSD/NVMe: << 1 s even for multi-TB. HDD: ~1–2 s per TiB. Small disks: instant.
 * - After mount, alloc/free paths are dominated by at most two L0 sector RMWs
 *   unless allocating whole groups, which are pure writes (no reads).
 * - The caches are small (≈ MB for multi-TB) and hot in CPU caches.
 *
 * Correctness & recovery
 * ----------------------
 * - L0 is the source of truth. If in doubt (e.g. after a crash), rebuild L1/L2
 *   by streaming L0 at mount; they will match the on-disk state.
 * - For extra safety in future, a tiny “pending bitmap op” journal could be added,
 *   but is not required for normal operation.
 *
 * Gotchas
 * -------
 * - Always compute the tail group’s `group_size` correctly (last group may be < G).
 * - Keep units straight (sectors vs bytes) in all I/O helpers.
 * - When changing G/S, revisit the assumption that one L1 group == one L0 map sector.
 *   The current code relies on G==4096 for that 1:1 mapping; it can be generalised later.
 *
 * Testing checklist
 * -----------------
 * - Allocate/free within a single group (various offsets/lengths).
 * - Allocate spanning multiple groups (tail + N full groups + head).
 * - Edge bytes/bit boundaries (bit 7->0 across L0 bytes).
 * - Tail group smaller than G.
 * - Rebuild caches and verify counters/bitsets match the on-disk L0.
 */


/**
 * @brief Update L2 cache state for a given L1 group.
 *
 * This function marks the free/used status of a specific L1 group in the
 * higher-level L2 bitsets. It is used whenever an extent changes state
 * (allocated or freed) so that the hierarchical free space map remains
 * consistent.
 *
 * The RFS free space tracking hierarchy has three levels:
 *   - L0: On-disk bitmap, one bit per sector, stored in the free space map.
 *   - L1: In-RAM summary of each group of @ref RFS_L1_GROUP_SECTORS sectors
 *             (4096 sectors = 2 MB at 512 B/sector). Each group tracks:
 *             - `l1_free_count[g]` - number of free sectors in the group.
 *             - `l1_not_full` bit - set if the group has any free space.
 *             - `l1_all_free` bit - set if the group is entirely free.
 *   - L2: In-RAM summary of super-groups, each covering
 *             @ref RFS_L2_GROUPS_PER_SUPER L1 groups. L2 bitsets indicate:
 *             - `l2_not_full` - at least one L1 group inside is not full.
 *             - `l2_all_free` - all L1 groups inside are fully free.
 *
 * This function recalculates the L2 state for the super-group containing
 * the specified L1 group index `g`. It must be called whenever
 * `l1_free_count[g]` changes due to marking or unmarking space.
 *
 * @param info Pointer to the mounted RFS volume structure.
 * @param g       Index of the L1 group whose parent L2 super-group
 *                    should be updated.
 *
 * @note
 * - Assumes @ref RFS_L1_GROUP_SECTORS is exactly 4096.
 * - Does not update the on-disk L0 bitmap - that is handled separately
 *   by the allocation/free functions.
 * - This is an `inline` helper and is not intended for direct external use.
 */
static inline void rfs_update_l2_for_group(rfs_t *info, uint64_t g) {
	const uint64_t S = RFS_L2_GROUPS_PER_SUPER;
	const uint64_t sg = g / S;
	const uint64_t g0 = sg * S;
	const uint64_t g1 = (g0 + S <= info->l1_groups) ? (g0 + S) : info->l1_groups;

	bool any_free = false;
	bool all_groups_all_free = true;

	for (uint64_t gg = g0; gg < g1; ++gg) {
		if (info->l1_free_count[gg] > 0) any_free = true;
		if (!bitset_get(info->l1_all_free, gg)) all_groups_all_free = false;
	}
	bitset_set(info->l2_not_full, sg, any_free);
	bitset_set(info->l2_all_free, sg, all_groups_all_free);
}

bool rfs_mark_extent(rfs_t *info, uint64_t start_sector, uint64_t length_sectors, bool mark_used) {
	if (!info || !info->desc || length_sectors == 0) {
		fs_set_error(FS_ERR_INVALID_ARG);
		dprintf("rfs_mark_extent: Invalid arg\n");
		return false;
	}
	if (info->desc->free_space_map_start == 0 || !info->l1_not_full || !info->l1_all_free || !info->l1_free_count || !info->l2_not_full || !info->l2_all_free || !info->l2_groups) {
		fs_set_error(FS_ERR_INVALID_ARG);
		dprintf("rfs_mark_extent: L1/L2 not initialised\n");
		return false;
	}

	const uint64_t total = info->total_sectors;
	if (start_sector >= total) {
		fs_set_error(FS_ERR_INVALID_ARG);
		dprintf("rfs_mark_extent: start sector out of range\n");
		return false;
	}
	if (start_sector + length_sectors > total) {
		length_sectors = total - start_sector; // clamp
		if (!length_sectors) {
			fs_set_error(FS_ERR_INVALID_GEOMETRY);
			dprintf("rfs_mark_extent: Invalid geometry\n");
			return false;
		}
	}

	const uint64_t G = RFS_L1_GROUP_SECTORS;        // 4096
	const uint64_t map_start = info->desc->free_space_map_start;

	uint8_t sector_buf[RFS_SECTOR_SIZE]; // single L0 sector RMW buffer

	uint64_t pos = start_sector;
	uint64_t rem = length_sectors;

	while (rem) {
		const uint64_t g = pos / G;
		const uint64_t g_start = g * G;
		const uint64_t g_end = (g_start + G <= total) ? (g_start + G) : total;
		const uint64_t g_size = g_end - g_start;
		const uint64_t in_group = g_end - pos;
		const uint64_t take = (rem < in_group) ? rem : in_group;
		const uint64_t l0_lba = map_start + g; // 1 L0 map sector per group

		if (take == g_size) {
			// Whole-group fast path: write all 1s (used) or all 0s (free).
			dprintf("rfs_mark_extent take == g_size, %p %lu\n", sector_buf, sizeof(sector_buf));
			memset(sector_buf, mark_used ? 0xFF : 0x00, sizeof(sector_buf));
			if (!rfs_write_device(info, l0_lba, RFS_SECTOR_SIZE, sector_buf)) {
				dprintf("rfs_mark_extent: write L0 sector %lu failed\n", l0_lba);
				return false;
			}

			// Update L1 counters & bitsets
			info->l1_free_count[g] = mark_used ? 0 : (uint16_t) g_size;
			bitset_set(info->l1_not_full, g, !mark_used);
			bitset_set(info->l1_all_free, g, !mark_used); // all free iff we're freeing whole group

			rfs_update_l2_for_group(info, g);
		} else {
			// Partial group: read-modify-write
			dprintf("rfs_mark_extent take != g_size [RMW], %p %lu\n", sector_buf, sizeof(sector_buf));
			if (!rfs_read_device(info, l0_lba, RFS_SECTOR_SIZE, sector_buf)) {
				dprintf("rfs_mark_extent: read L0 sector %lu failed\n", l0_lba);
				return false;
			}

			const uint64_t local_start = pos - g_start;
			const uint64_t local_end = local_start + take;

			uint64_t transitions = 0; // number of bits that actually change (0->1 or 1->0)

			uint64_t b = local_start;
			while (b < local_end) {
				const uint64_t byte_idx = b >> 3;
				const uint32_t bit_off = (uint32_t) (b & 7ULL);
				const uint64_t span = MIN((uint64_t) 8 - bit_off, local_end - b);
				const uint8_t mask = (uint8_t) (((1u << span) - 1u) << bit_off);

				const uint8_t before = sector_buf[byte_idx];
				const uint8_t after = mark_used ? (uint8_t) (before | mask) : (uint8_t) (before & ~mask);
				sector_buf[byte_idx] = after;

				// Count transitions:
				// used: newly set = (~before) & after & mask == (~before) & mask
				// free: newly clr = before & (~after) & mask == before & mask
				const uint8_t delta = mark_used ? (uint8_t) ((~before) & mask) : (uint8_t) (before & mask);
				transitions += __builtin_popcount((unsigned) delta);
				b += span;
			}

			if (transitions) {
				dprintf("rfs_mark_extent: prep write %lu %u\n", l0_lba, RFS_SECTOR_SIZE);
				if (!rfs_write_device(info, l0_lba, RFS_SECTOR_SIZE, sector_buf)) {
					dprintf("rfs_mark_extent: write L0 sector %lu failed\n", l0_lba);
					return false;
				}

				// Update L1 counters & bitsets
				uint16_t *fc = &info->l1_free_count[g];
				if (mark_used) {
					// free_count decreases by the number of 0->1 transitions
					*fc = (*fc >= transitions) ? (uint16_t) (*fc - transitions) : 0;
				} else {
					// freeing: free_count increases, cap at g_size
					uint64_t room = g_size - *fc;
					*fc += (uint16_t) MIN((uint64_t) transitions, room);
				}

				bitset_set(info->l1_not_full, g, (*fc > 0));
				bitset_set(info->l1_all_free, g, (*fc == (uint16_t) g_size));

				rfs_update_l2_for_group(info, g);
			}
		}

		pos += take;
		rem -= take;
	}

	return true;
}

/**
 * @brief Get the number of sectors in a given L1 group.
 *
 * Returns the size of the specified L1 group in sectors.
 * For most groups this will be @ref RFS_L1_GROUP_SECTORS, but
 * the final (tail) group may be smaller if the total sector
 * count is not a multiple of the group size.
 *
 * @param info Pointer to the RetroFS volume structure.
 * @param g    L1 group index.
 * @return     Number of sectors in the group.
 */static inline uint64_t rfs_group_size(const rfs_t *info, uint64_t g) {
	const uint64_t G = RFS_L1_GROUP_SECTORS;
	const uint64_t start = g * G;
	const uint64_t total = info->total_sectors;
	return (start + G <= total) ? G : (total - start);
}

/**
 * @brief Load the L0 bitmap sector for a given L1 group.
 *
 * Reads the on-disk free space bitmap sector corresponding to the
 * specified L1 group index into the provided buffer. There is a
 * 1:1 mapping between L1 groups and bitmap sectors when
 * @ref RFS_L1_GROUP_SECTORS is 4096.
 *
 * @param info Pointer to the RetroFS volume structure.
 * @param g    L1 group index to load.
 * @param buf  Destination buffer for the sector data (must be at least
 *             @ref RFS_SECTOR_SIZE bytes).
 * @return     true on success, false on read error.
 */
static inline bool rfs_load_group_sector(rfs_t *info, uint64_t g, uint8_t *buf) {
	const uint64_t l0_lba = info->desc->free_space_map_start + g;
	return rfs_read_device(info, l0_lba, RFS_SECTOR_SIZE, buf) != 0;
}

/**
 * @brief Count contiguous FREE (0) bits at the start and end of an L1 group's L0 bitmap sector.
 *
 * Scans the on-disk bitmap for a given L1 group to determine:
 * - The length of the leading run of free sectors (prefix).
 * - The length of the trailing run of free sectors (suffix).
 *
 * These values are useful for quickly evaluating whether an extent
 * may span multiple L1 groups without fragmentation.
 *
 * @param buf         Pointer to the L0 bitmap sector buffer for the group.
 * @param group_bits  Number of valid bits in the group (may be less than
 *                    @ref RFS_L1_GROUP_SECTORS for the last group).
 * @param out_prefix  Output pointer for the number of free bits from the start.
 * @param out_suffix  Output pointer for the number of free bits from the end.
 */
static void rfs_group_prefix_suffix_free(const uint8_t *buf, uint64_t group_bits, uint64_t *out_prefix, uint64_t *out_suffix) {
	uint64_t p = 0, s = 0, b = 0;

	// Prefix
	while (b + 8 <= group_bits) {
		uint8_t byte = buf[b >> 3];
		if (byte == 0x00) {
			p += 8;
			b += 8;
		} else {
			uint8_t x = byte;
			while (b < group_bits && !(x & 1u)) {
				++p;
				++b;
				x >>= 1;
			}
			break;
		}
	}
	while (b < group_bits) {
		uint8_t byte = buf[b >> 3];
		uint8_t bit = (uint8_t) ((byte >> (b & 7)) & 1u);
		if (bit == 0) {
			++p;
			++b;
		} else {
			break;
		}
	}

	// Suffix
	if (group_bits > 0) {
		int64_t b = (int64_t) group_bits - 1;
		while (b >= 0) {
			uint8_t byte = buf[(uint64_t) b >> 3];
			uint8_t bit = (uint8_t) ((byte >> (b & 7)) & 1u);
			if (bit == 0) {
				++s;
				--b;
			} else {
				break;
			}

			if (((b + 1) & 7) == 0) {
				int64_t byte_idx = ((b >> 3) - 1);
				while (byte_idx >= 0 && ((uint8_t) buf[byte_idx] == 0x00)) {
					s += 8;
					b -= 8;
					--byte_idx;
				}
			}
		}
	}

	*out_prefix = p;
	*out_suffix = s;
}

/**
 * @brief Find the first contiguous run of FREE (0) bits of at least a given length.
 *
 * Searches within an L0 bitmap sector for a run of free sectors starting
 * from a specified bit offset. Returns the local bit index of the first
 * sector in the run, or @c UINT64_MAX if no suitable run is found.
 *
 * @param buf         Pointer to the L0 bitmap sector buffer for the group.
 * @param group_bits  Number of valid bits in this group (may be smaller than
 *                    @ref RFS_L1_GROUP_SECTORS for the last group).
 * @param offset      Local bit index within the group to begin the search.
 * @param need        Required length of the run in bits (sectors).
 * @return Local bit index of the first sector in the run, or @c UINT64_MAX if not found.
 */
static uint64_t rfs_group_find_run_from(const uint8_t *buf, uint64_t group_bits, uint64_t offset, uint64_t need) {
	uint64_t run = 0, run_start = offset;
	uint64_t b;

	if (need == 0) {
		return offset;
	}
	if (offset >= group_bits) {
		return (uint64_t) -1;
	}

	for (b = offset; b < group_bits; ++b) {
		uint8_t byte = buf[b >> 3];
		uint8_t bit = (uint8_t) ((byte >> (b & 7)) & 1u); // 0=free, 1=used
		if (bit == 0) {
			if (run == 0) {
				run_start = b;
			}
			if (++run >= need) {
				return run_start;
			}
		} else {
			run = 0;
		}
	}
	return (uint64_t) -1;
}

bool rfs_find_free_extent(rfs_t *info, uint64_t need, uint64_t *out_start_sector) {
	uint64_t total, G, S;

	if (!info || !info->desc || !out_start_sector || need == 0) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return false;
	}

	total = info->total_sectors;
	if (need > total) {
		fs_set_error(FS_ERR_NO_SPACE);
		return false;
	}

	G = RFS_L1_GROUP_SECTORS;        // 4096
	S = RFS_L2_GROUPS_PER_SUPER;

	// ----- Case A: need fits within a single group -----
	if (need <= G) {
		uint64_t sg;
		for (sg = 0; sg < info->l2_groups; ++sg) {
			if (!bitset_get(info->l2_not_full, sg)) {
				continue;
			}

			const uint64_t g0 = sg * S;
			const uint64_t g1 = MIN(g0 + S, info->l1_groups);
			uint64_t g;

			for (g = g0; g < g1; ++g) {
				uint64_t gs;

				if (!bitset_get(info->l1_not_full, g)) {
					continue; // full
				}

				gs = rfs_group_size(info, g);

				if (bitset_get(info->l1_all_free, g)) {
					if (need <= gs) {
						*out_start_sector = g * G;
						return true;
					}
					continue;
				}

				// Mixed: read L0 once and scan
				uint8_t l0[RFS_SECTOR_SIZE];
				uint64_t local;
				if (!rfs_load_group_sector(info, g, l0)) {
					return false;
				}

				local = rfs_group_find_run_from(l0, gs, 0, need);
				if (local != (uint64_t) -1) {
					*out_start_sector = g * G + local;
					return true;
				}
			}
		}
		fs_set_error(FS_ERR_NO_SPACE);
		return false;
	}

	// Case B: need spans multiple groups
	{
		const uint64_t remainder = need % G;
		uint64_t sg;

		for (sg = 0; sg < info->l2_groups; ++sg) {
			if (!bitset_get(info->l2_not_full, sg)) {
				continue;
			}

			const uint64_t g0 = sg * S;
			const uint64_t g1 = MIN(g0 + S, info->l1_groups);
			uint64_t g;

			for (g = g0; g < g1; ++g) {
				uint64_t gs, tail_free = 0;
				uint64_t start_here;
				uint64_t collected;
				uint64_t gg;

				// Determine tail-free in group g
				gs = rfs_group_size(info, g);

				if (bitset_get(info->l1_all_free, g)) {
					tail_free = gs;
				} else if (bitset_get(info->l1_not_full, g)) {
					uint8_t l0[RFS_SECTOR_SIZE];
					uint64_t prefix, suffix;
					if (!rfs_load_group_sector(info, g, l0)) return false;
					rfs_group_prefix_suffix_free(l0, gs, &prefix, &suffix);
					(void) prefix;
					tail_free = suffix;
				} else {
					continue; // group full
				}

				if (tail_free == 0) {
					continue;
				}

				start_here = g * G + (gs - tail_free);
				collected = tail_free;

				// Chain fully-free groups after g
				gg = g + 1;
				while (collected < need && gg < info->l1_groups && bitset_get(info->l1_all_free, gg)) {
					collected += rfs_group_size(info, gg);
					++gg;
				}

				if (collected >= need) {
					*out_start_sector = start_here;
					return true;
				}

				// Try to finish in next group’s prefix, if any remainder needed
				if (remainder > 0 && collected < need && gg < info->l1_groups) {
					uint64_t next_gs, prefix_free = 0;

					next_gs = rfs_group_size(info, gg);

					if (bitset_get(info->l1_all_free, gg)) {
						prefix_free = next_gs;
					} else if (bitset_get(info->l1_not_full, gg)) {
						uint8_t l0n[RFS_SECTOR_SIZE];
						uint64_t pref, suff;
						if (!rfs_load_group_sector(info, gg, l0n)) {
							return false;
						}
						rfs_group_prefix_suffix_free(l0n, next_gs, &pref, &suff);
						(void) suff;
						prefix_free = pref;
					} else {
						prefix_free = 0;
					}

					if (collected + prefix_free >= need) {
						*out_start_sector = start_here;
						return true;
					}
				}
			}
		}
	}

	fs_set_error(FS_ERR_NO_SPACE);
	return false;
}

static void rfs_free_level_caches(rfs_t *info) {
	if (!info) {
		return;
	}
	kfree_null(&info->cache_block);
	info->cache_block_size = 0;
	info->l1_free_count = NULL;
	info->l1_not_full = NULL;
	info->l1_all_free = NULL;
	info->l2_not_full = NULL;
	info->l2_all_free = NULL;
}

bool rfs_build_level_caches(rfs_t *info) {
	if (!info || !info->desc) {
		return false;
	}

	/* Geometry */
	dprintf("RFS build level caches sectors = %lu\n", info->total_sectors);

	const uint64_t G = RFS_L1_GROUP_SECTORS;       /* sectors per L1 group (e.g., 4096) */
	const uint64_t S = RFS_L2_GROUPS_PER_SUPER;    /* L1 groups per L2 super-group */

	info->l1_groups = (info->total_sectors + G - 1ULL) / G;
	info->l2_groups = (info->l1_groups + S - 1ULL) / S;

	/* ---- single-slab allocation for all L1/L2 structures + build buffer ---- */
	const size_t l1_bytes = bitset_bytes(info->l1_groups);
	const size_t l2_bytes = bitset_bytes(info->l2_groups);

	const size_t sz_l1_free_count = info->l1_groups * sizeof(uint16_t);
	const size_t sz_l1_not_full = l1_bytes;
	const size_t sz_l1_all_free = l1_bytes;

	const size_t sz_l2_not_full = l2_bytes;
	const size_t sz_l2_all_free = l2_bytes;

	const size_t max_bytes_per_read = (size_t) (RFS_MAP_READ_CHUNK_SECTORS * RFS_SECTOR_SIZE);

	const size_t AL = 8; /* conservative alignment */

	size_t off = 0;
	const size_t off_l1_free_count = ALIGN_UP(off, AL);
	off = off_l1_free_count + sz_l1_free_count;
	const size_t off_l1_not_full = ALIGN_UP(off, AL);
	off = off_l1_not_full + sz_l1_not_full;
	const size_t off_l1_all_free = ALIGN_UP(off, AL);
	off = off_l1_all_free + sz_l1_all_free;
	const size_t off_l2_not_full = ALIGN_UP(off, AL);
	off = off_l2_not_full + sz_l2_not_full;
	const size_t off_l2_all_free = ALIGN_UP(off, AL);
	off = off_l2_all_free + sz_l2_all_free;
	const size_t off_buf = ALIGN_UP(off, AL);
	off = off_buf + max_bytes_per_read;

	const size_t total_bytes = off;

	void *block = kmalloc(total_bytes);
	if (!block) {
		dprintf("rfs_build_level_caches: OOM cache slab (%lu bytes)\n", total_bytes);
		return false;
	}
	dprintf("rfs_build_level_caches: cache slab %lu bytes\n", total_bytes);

	/* Publish owning pointer for later teardown */
	info->cache_block = block;
	info->cache_block_size = total_bytes;

	/* Carve out sub-buffers */
	info->l1_free_count = (uint16_t *) ((uint8_t *) block + off_l1_free_count);
	info->l1_not_full = (uint8_t *) ((uint8_t *) block + off_l1_not_full);
	info->l1_all_free = (uint8_t *) ((uint8_t *) block + off_l1_all_free);
	info->l2_not_full = (uint8_t *) ((uint8_t *) block + off_l2_not_full);
	info->l2_all_free = (uint8_t *) ((uint8_t *) block + off_l2_all_free);
	uint8_t *buf = ((uint8_t *) block + off_buf);

	dprintf("setup l1/l2 cache blocks");

	/* Zero initial state */
	memset(info->l1_free_count, 0, sz_l1_free_count);
	memset(info->l1_not_full, 0, sz_l1_not_full);
	memset(info->l1_all_free, 0, sz_l1_all_free);
	memset(info->l2_not_full, 0, sz_l2_not_full);
	memset(info->l2_all_free, 0, sz_l2_all_free);

	dprintf("l1/l2 clear\n");

	/* Stream the Level-0 bitmap (on disk: 1 bit per sector, 1=used, 0=free) */
	const uint64_t map_start = info->desc->free_space_map_start;
	const uint64_t map_len = info->desc->free_space_map_length;
	const uint64_t total_bits = info->total_sectors;

	uint64_t bit_cursor = 0;   /* global sector-index represented by next L0 bit */
	uint64_t sector_off = 0;   /* offset into L0 map (in sectors) */

	dprintf("l1/l2 start read\n");

	while ((sector_off < map_len) && (bit_cursor < total_bits)) {
		const uint64_t remaining = map_len - sector_off;
		const uint64_t sectors_this = MIN(remaining, RFS_MAP_READ_CHUNK_SECTORS);
		if (sectors_this == 0) {
			break;
		}

		const size_t bytes_this = (size_t) (sectors_this * RFS_SECTOR_SIZE);

		//kprintf("fsmap read l0 ms+off=%lu bytes=%lu buf=%p off=%lu mlen=%lu\n", map_start + sector_off, bytes_this, buf, sector_off, map_len);
		if (!rfs_read_device(info, map_start + sector_off, bytes_this, buf)) {
			kprintf("rfs_build_level_caches: failed read @LBA %lu (sectors=%lu)\n", map_start + sector_off, sectors_this);
			rfs_free_level_caches(info);
			return false;
		}

		/* Valid bits in this buffer (do not run past end of volume) */
		const uint64_t bits_in_buf = MIN((uint64_t) bytes_this * 8ULL, total_bits - bit_cursor);

		/* Process 64-bit words of bits; invert so 1-bits mean "free". */
		const uint64_t full_words = bits_in_buf >> 6;   /* / 64 */
		const uint64_t tail_bits = bits_in_buf & 63ULL;

		const uint64_t *wptr = (const uint64_t *) buf;

		uint64_t pos = bit_cursor; /* global bit index for start of current word */

		/* Full 64-bit words */
		for (uint64_t wi = 0; wi < full_words; ++wi, ++wptr, pos += 64ULL) {
			uint64_t word = ~(*wptr); /* free=1, used=0 */

			if (word == 0ULL) {
				continue;
			}

			/* Distribute this word’s free bits across L1 groups it may span */
			uint64_t remaining_bits = 64ULL;
			while ((word != 0ULL) && (remaining_bits != 0ULL)) {
				const uint64_t g = pos / G;
				if (g >= info->l1_groups) {
					/* Safety; should not happen because bits_in_buf clamps to total_bits */
					break;
				}

				const uint64_t group_end = (g + 1ULL) * G;
				const uint64_t in_group = group_end - pos;          /* bits to group boundary */
				const uint64_t take = MIN(remaining_bits, in_group); /* <=64 */

				/* Mask off only the 'take' LSBs of 'word' */
				const uint64_t mask = (take == 64ULL) ? ~0ULL : ((1ULL << take) - 1ULL);
				const uint64_t seg = word & mask;

				if (seg != 0ULL) {
					const int add = __builtin_popcountll(seg);
					uint16_t *fc = &info->l1_free_count[g];

					const uint64_t group_start = g * G;
					uint64_t group_size;
					if ((group_start + G) <= info->total_sectors) {
						group_size = G;
					} else {
						group_size = info->total_sectors - group_start;
					}

					if (*fc < group_size) {
						const uint64_t room = group_size - *fc;
						const uint64_t inc = MIN((uint64_t) add, room);
						*fc = (uint16_t) (*fc + (uint16_t) inc);
					}
				}

				/* Advance within word and possibly into next group */
				word >>= take;
				pos += take;
				remaining_bits -= take;
			}
		}

		/* Tail bits (0..63) */
		if (tail_bits != 0ULL) {
			const uint64_t tail_word = ((const uint64_t *) buf)[full_words];
			uint64_t word = ~tail_word;

			/* Keep only 'tail_bits' */
			const uint64_t mask_tail = (tail_bits == 64ULL) ? ~0ULL : ((1ULL << tail_bits) - 1ULL);
			word &= mask_tail;

			uint64_t remaining_bits = tail_bits;
			while ((word != 0ULL) && (remaining_bits != 0ULL)) {
				const uint64_t g = pos / G;
				if (g >= info->l1_groups) {
					break;
				}

				const uint64_t group_end = (g + 1ULL) * G;
				const uint64_t in_group = group_end - pos;
				const uint64_t take = MIN(remaining_bits, in_group);

				const uint64_t seg_mask = (take == 64ULL) ? ~0ULL : ((1ULL << take) - 1ULL);
				const uint64_t seg = word & seg_mask;

				if (seg != 0ULL) {
					const int add = __builtin_popcountll(seg);
					uint16_t *fc = &info->l1_free_count[g];

					const uint64_t group_start = g * G;
					uint64_t group_size;
					if ((group_start + G) <= info->total_sectors) {
						group_size = G;
					} else {
						group_size = info->total_sectors - group_start;
					}

					if (*fc < group_size) {
						const uint64_t room = group_size - *fc;
						const uint64_t inc = MIN((uint64_t) add, room);
						*fc = (uint16_t) (*fc + (uint16_t) inc);
					}
				}

				word >>= take;
				pos += take;
				remaining_bits -= take;
			}
		}

		bit_cursor += bits_in_buf;
		sector_off += sectors_this;
	}

	dprintf("l1/l2 done read\n");

	/* Derive L1 bitsets from counters */
	for (uint64_t g = 0; g < info->l1_groups; ++g) {
		const uint16_t fc = info->l1_free_count[g];
		bitset_set(info->l1_not_full, g, (fc > 0));

		const uint64_t group_start = g * G;
		uint64_t group_size;
		if ((group_start + G) <= info->total_sectors) {
			group_size = G;
		} else {
			group_size = info->total_sectors - group_start;
		}
		bitset_set(info->l1_all_free, g, (fc == (uint16_t) group_size));
	}

	dprintf("l1 bitset\n");

	/* Build L2 by folding over S children */
	for (uint64_t sg = 0; sg < info->l2_groups; ++sg) {
		const uint64_t g0 = sg * S;
		const uint64_t g1 = MIN(g0 + S, info->l1_groups);

		bool any_free = false;
		bool all_full_groups_free = true;

		for (uint64_t g = g0; g < g1; ++g) {
			if (bitset_get(info->l1_not_full, g)) {
				any_free = true;
			}
			if (!bitset_get(info->l1_all_free, g)) {
				all_full_groups_free = false;
			}
		}

		bitset_set(info->l2_not_full, sg, any_free);
		bitset_set(info->l2_all_free, sg, all_full_groups_free);
	}

	dprintf("RFS: Built L1/L2: total_sectors=%lu, l1_groups=%lu, l2_groups=%lu (slab=%lu bytes)\n", info->total_sectors, info->l1_groups, info->l2_groups, total_bytes);
	return true;
}
