# RetroFS v1 Technical Specification </h1>

| | |
|------------|-------|
| **Status:** | Draft |
| **Version:** | 1.0  |
| **Audience:** | System implementors / filesystem driver authors |
| **Scope:** | On-disk format and required behaviours for RetroFS v1 |
| | | 

## 1. Conventions and terminology

* The key words **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY** are to be interpreted as in RFC 2119.
* All multi-byte integers on disk **MUST** be **little-endian**.
* A **sector** is exactly **512 bytes** (v1 only; other sizes are not supported).
* An **extent** is a contiguous run of sectors.
* A **directory block** is a contiguous run of **`RFS_DEFAULT_DIR_SIZE`** sectors (v1: 64).
* A **half-sector entry** is 256 bytes; two entries per 512-byte sector.
* Filenames are **case-preserving**, compared **case-insensitively** using ASCII rules only.

## 2. High-level layout (informative)

```
LBA 0                 : Description Block
LBA root_directory .. : Root directory block (64 sectors) + any continuations
LBA free_space_map .. : Free-space map (bitmap), contiguous
LBA …                 : File and directory extents
```

Exact locations are recorded in the Description Block.

## 3. On-disk structures (normative)

### 3.1 Description Block (sector 0)

The Description Block **MUST** occupy exactly one sector at LBA 0 and uses this exact layout:

| Offset | Size | Field                     | Type     | Description                                                        |
| -----: | ---: | ------------------------- | -------- | ------------------------------------------------------------------ |
|   0x00 |    8 | `identifier`              | `u64`    | Magic **`0x3153466f72746552`** (“RetroFS1”).                       |
|   0x08 |    8 | `root_directory`          | `u64`    | LBA of the **directory start entry** for the root directory.       |
|   0x10 |    8 | `free_space_map_start`    | `u64`    | LBA of the first sector of the free-space map.                     |
|   0x18 |    8 | `free_space_map_length`   | `u64`    | Length of the free-space map, in sectors.                          |
|   0x20 |    8 | `free_space_map_checksum` | `u64`    | Checksum of the free-space map (algorithm implementation-defined). |
|   0x28 |    8 | `sequence`                | `u64`    | Filesystem-wide sequence; MAY increment on metadata changes.       |
|   0x30 |    8 | `creation_time`           | `time_t` | POSIX time (UTC) when the FS was created.                          |
|   0x38 |    … | padding/reserved          | bytes    | Zero-filled to 512 bytes.                                          |

**Rules**

* `identifier` **MUST** match or the volume **MUST NOT** mount.
* All pointer/length fields **MUST** reference sectors within the partition/device bounds.
* Unused bytes **MUST** be zero and **SHOULD** be preserved on rewrite.

#### Byte layout sketch

```
+0000  52 65 74 72 6F 46 53 31   // "RetroFS1" LE: 0x3153466f72746552
+0008  <root_directory: u64 LE>
+0010  <fsmap_start:    u64 LE>
+0018  <fsmap_length:   u64 LE>
+0020  <fsmap_cksum:    u64 LE>
+0028  <sequence:       u64 LE>
+0030  <creation_time:  time_t LE>
+0038  ... zero to 0x200
```

### 3.2 Free-space map (bitmap)

Each free-space map sector uses:

```
typedef struct {
    uint64_t bits[ RFS_FS_MAP_BITS_PER_SECTOR ]; // (512 / 8) = 64 elements
} rfs_free_space_map_part_t;
```

* One map sector encodes **64 × 64 = 4096 sectors** of allocation state.
* Bit value **1** = allocated, **0** = free.
* Bit order within each `uint64_t` is the normal LE bit order (LSB is the lowest sector index of that word).

**Rules**

* The map **MUST** cover the addressable sector range of the filesystem.
* Updates **SHOULD** be atomic at sector granularity (read–modify–write the containing map sector).
* Implementations **SHOULD** cache higher-level summaries in RAM (see §7).

### 3.3 Directory blocks and entries

A directory is stored in one or more directory blocks. Each block is exactly **`RFS_DEFAULT_DIR_SIZE`** sectors (v1: **64**). Each **sector** contains **two half-sector entries**.

#### 3.3.1 Directory Start entry (first half-sector of the block)

```c
typedef struct rfs_directory_start_t {
    uint32_t flags;          // MUST include RFS_FLAG_DIR_START
    char     title[128];     // NUL-terminated if shorter
    uint64_t parent;         // LBA of parent directory's start entry
    uint64_t sectors;        // size of this directory block in sectors (v1: 64)
    uint64_t continuation;   // LBA of next directory block, or 0
    char     reserved[];     // zero-filled to 256 bytes
} __attribute__((packed));
```

**Rules**

* The very first entry in the block **MUST** be a Directory Start entry with `RFS_FLAG_DIR_START` set.
* `sectors` **MUST** equal `RFS_DEFAULT_DIR_SIZE` in v1.
* `continuation` **MUST** be 0 if no further blocks exist; otherwise it points to the next block’s first sector.

#### 3.3.2 File/Directory entry (other half-sectors)

```c
typedef struct rfs_directory_entry_inner_t {
    uint32_t flags;              // RFS_FLAG_DIRECTORY et al.
    char     filename[128];      // NUL-terminated
    uint64_t sector_start;       // LBA of payload (file) or start (subdir)
    uint64_t length;             // logical length in bytes (0 for directories)
    uint64_t sector_length;      // reserved capacity in sectors
    time_t   created;            // UTC
    time_t   modified;           // UTC
    uint64_t sequence;           // per-entry version counter
    char     reserved[];         // zero-filled to 256 bytes total
} __attribute__((packed));
```

#### 3.3.3 File/Directory Flags values

Files and directories may set the following bits in their `flags` field:

| Flag Name           | Bit Position | Description |
|---------------------|--------------|-------------|
| RFS_FLAG_DIRECTORY  | 0 (0x01)     | This flag **MUST** be set if the entry points at a subdirectory. |
| RFS_FLAG_LOCKED     | 1 (0x02)     | This flag **MAY** be set to indicate the file is locked against accidental change. Its behaviour is implementation specific |
| RFS_FLAG_DIR_START  | 2 (0x04)     | This flag **MUST** be set in the directory start entry at the start of each directory block. Directory start blocks without this flag set should be considered invalid and not parsed. |

**Rules**

* Filenames are **case-preserving**, compared **case-insensitively** (ASCII only).
* Unused slots **MUST** be marked by `filename[0] == 0`.
* For directories, `flags` **MUST** include `RFS_FLAG_DIRECTORY` and `length` **MUST** be 0.
* `sector_length` is the reserved extent size; `length` **MUST NOT** exceed `sector_length * 512`.
* `sequence` is a human-oriented revision counter: implementations **SHOULD** increment it on payload-modifying writes and **MAY** leave it unchanged on metadata-only updates.

#### 3.3.3 Directory walk

* Readers **MUST** follow the `continuation` chain until `0`.
* Readers **MUST** bound the walk (implementation-defined limit) to avoid infinite loops on corruption.
* Within each block, readers **MAY** stop at the first `filename[0] == 0` (end-of-entries for that block).

## 4. Allocation & growth semantics (normative)

### 4.1 Extents

* Every file and directory occupies exactly one **contiguous** extent.
* When a file write would exceed its reserved capacity (`sector_length * 512`), the file **MUST** be **relocated** to a larger contiguous extent (“extend-and-move”), after which the old extent **MUST** be freed.

### 4.2 Creation

**Files**

* Allocate an extent of at least the chosen reservation (policy; see §6.1).
* **MUST** zero-fill the entire reserved extent before exposing it.
* Insert/update the parent directory entry with:

  * `sector_start` = allocated LBA
  * `sector_length` = reserved sectors
  * `length` = initial logical size (bytes)
  * `flags` without `RFS_FLAG_DIRECTORY`

**Directories**

* Allocate one directory block (`RFS_DEFAULT_DIR_SIZE` sectors).
* **MUST** zero-fill the block.
* Write a valid Directory Start entry (`RFS_FLAG_DIR_START`, correct `parent`, `sectors`, `continuation=0`).
* Insert a parent entry with `RFS_FLAG_DIRECTORY` set and `length=0`.

### 4.3 Writing

* For unaligned writes, drivers **MUST** perform head/tail read-modify-write and full-sector writes in the middle.
* If `start + length` would exceed reserved capacity, driver **MUST** extend-and-move before writing.
* On successful payload modification, drivers **SHOULD** bump the per-entry `sequence`.

### 4.4 Truncation

* Truncate **MUST** update `length` only.
* Truncate **MUST NOT** free sectors nor reduce `sector_length`.
* If requested `length` exceeds reserved capacity, truncate **MUST** fail.

### 4.5 Deletion

**Files**

* Remove the directory entry (compacting the block to avoid holes).
* Free the entire `sector_length` span in the free-space map.

**Directories**

* Refuse deletion unless the directory and **all** continuation blocks contain no valid entries.
* Remove the parent entry first, then free each directory block in the chain (each `sectors` long).

## 5. Name handling

* Case-preserving storage; ASCII case-insensitive matching.
* Two names differing only by ASCII case **MUST** be considered equal and refer to the same entry.
* Names **MUST** be NUL-terminated within `RFS_MAX_NAME` bytes (128 incl. terminator).

## 6. Policy notes (non-normative but recommended)

### 6.1 Initial reservation policy

Implementations **SHOULD** reserve more than the initial logical size to reduce churn. A practical policy:

* Default reservation: **1 MiB**.
* Image file types (e.g., `jpg`, `jpeg`, `png`, `gif`, `tiff`, `bmp`, `webp`): **4 MiB**.
* The reservation **MAY** be adjusted later by extend-and-move when required.

### 6.2 Zero-fill

New extents and extended regions **MUST** be zero-filled prior to being visible to reads. This prevents data disclosure.

## 7. Implementation guidance: L1/L2 allocation caches

Although the in-memory mount context (`rfs_t`) is implementation-specific and out of scope for this document, a compliant, high-performance implementation **SHOULD** maintain multi-level summaries over the on-disk free-space map:

* **L1 groups** summarise `RFS_L1_GROUP_SECTORS` consecutive sectors (v1 default: **4096 sectors ≈ 2 MiB**).
  Track a per-group free count and bitsets for “any free” and “all free”.

* **L2 super-groups** summarise `RFS_L2_GROUPS_PER_SUPER` L1 groups (v1 default: **1024 L1s**, i.e. \~**2 GiB** span per super-group).
  Track bitsets for “any free” and “all free”.

**Recommendations**

* These defaults balance RAM overhead and search speed; implementations **MAY** tune them to workload.
* Total cache memory will typically be on the order of **\~200 MB per TiB** of formatted capacity; systems with constrained RAM **MAY** reduce group sizes at the cost of slower allocation.
* Where applicable, align L1 group size to device erase/stripe sizes to reduce write amplification.

## 8. Validation & error handling

* Mount **MUST** verify `identifier`, basic pointer ranges, and that root directory’s start entry has `RFS_FLAG_DIR_START` and `sectors == RFS_DEFAULT_DIR_SIZE`.
* Directory walks **MUST** enforce an iteration cap to avoid loops on corruption.
* All I/O routines **MUST** fail cleanly on bounds violations.
* On partial failure during create/extend, implementations **SHOULD** prefer leaving the new extent marked used (space leak) over risking a dangling metadata reference.

## 9. Partition identification (GPT)

### RetroFS Partition Type GUID

**Name:** RetroFS Partition
**Use:** Identifies a GPT partition formatted with the RetroFS filesystem.
**GUID (UUID):** `4DEC1156-FEC8-4495-854B-20D888E21AF0`
**Format:** UUID v4 (random; permanently reserved for RetroFS).
**Endianness:** Stored in GPT entries as per UEFI specification (little-endian in the first three fields).

**Operating System Usage**

* Retro Rocket **will** use this type GUID to auto-probe and mount RetroFS volumes.
* Other operating systems are not expected to recognise this type by default.

**Example GPT entry**

```
Partition GUID code: 4DEC1156-FEC8-4495-854B-20D888E21AF0 (RetroFS partition)
Partition unique GUID: [per-volume unique UUID]
First sector: N
Last sector: M
Partition size: [M−N+1] sectors
Partition name: 'RetroFS'
```

**Kernel header**

```c
/**
 * @brief RetroFS GPT Partition Type GUID
 * UUID v4 reserved for Retro Rocket RetroFS partitions
 */
#define RFS_GPT_GUID "4DEC1156-FEC8-4495-854B-20D888E21AF0"
```

## 10. Security considerations

* Zero-fill on allocation/extension prevents disclosure of previous contents.
* Reads **MUST NOT** expose bytes beyond `length` even if reserved space is larger.
* There is no journaling; callers **SHOULD** expect non-atomic metadata updates.

## 11. Versioning

* This document defines **RetroFS v1** with **512-byte sectors** and fixed directory block size **64 sectors**.
* Implementations **MUST** reject incompatible identifiers or field values that violate these invariants.
