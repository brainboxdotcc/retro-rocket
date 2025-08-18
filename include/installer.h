#pragma once
#include <kernel.h>

/**
 * @brief ASCII signature at LBA 1 for GPT headers
 */
#define GPT_SIGNATURE_TEXT "EFI PART"

/**
 * @brief GPT spec revision 1.0 (UEFI 2.0)
 */
#define GPT_REVISION_1_0 0x00010000U

/**
 * @brief GPT header size in bytes
 */
#define GPT_HEADER_SIZE_BYTES 92U

/**
 * @brief Number of GPT partition entries
 */
#define GPT_PTE_COUNT 128U

/**
 * @brief Size of each GPT partition entry in bytes
 */
#define GPT_PTE_SIZE_BYTES 128U

/**
 * @brief 1 MiB alignment boundary (2048 LBAs at 512 B each)
 */
#define ALIGN_1M_IN_LBAS 2048ULL

/**
 * @brief Required sector size (bytes per LBA)
 */
#define SECTOR_BYTES_REQUIRED 512U

/**
 * @brief Path to prebuilt FAT32 EFI System Partition image
 */
#define EFI_FAT_IMAGE "/efi.fat"

/**
 * @brief Mapping of a GPT GUID to user-facing labels
 */
typedef struct {
	const char *guid_text;
	const char *tag;
	const char *likely_os;
} gpt_guid_map_t;

/**
 * @brief Mapping of an MBR partition type byte to user-facing labels
 */
typedef struct {
	uint8_t mbr_type;
	const char *label;
	const char *os;
} mbr_id_map_t;

/**
 * @brief Known GPT partition type GUIDs of interest
 */
static const gpt_guid_map_t gpt_guid_map[] = {
	{ GPT_EFI_SYSTEM,                               "ESP",                NULL },
	{ RFS_GPT_GUID,                                 "RetroFS",            "Retro Rocket" },
	{ "E3C9E316-0B5C-4DB8-817D-F92DF00215AE",       "MSR",                NULL },
	{ "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7",       "Windows",            "Windows" },
	{ "DE94BBA4-06D1-4D40-A16A-BFD50179D6AC",       "Windows Recovery",   "Windows" },
	{ "E75CAF8F-F680-4CEE-AFA3-B001E56EFC2D",       "Windows Storage",    "Windows" },
	{ "0FC63DAF-8483-4772-8E79-3D69D8477DE4",       "Linux",              "Linux" },
	{ "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F",       "Linux Swap",         "Linux" },
	{ "A19D880F-05FC-4D3B-A006-743F0F84911E",       "Linux RAID",         "Linux" },
	{ "E6D6D379-F507-44C2-A23C-238F2A3DF928",       "Linux LVM",          "Linux" },
	{ "933AC7E1-2EB4-4F13-B844-0E14E2AEF915",       "Linux /home",        "Linux" },
	{ "83BD6B9D-7F41-11DC-B009-0019D1879648",       "FreeBSD Boot",       "FreeBSD" },
	{ "516E7CB4-6ECF-11D6-8FF8-00022D09712B",       "FreeBSD UFS",        "FreeBSD" },
	{ "516E7CB8-6ECF-11D6-8FF8-00022D09712B",       "FreeBSD ZFS",        "FreeBSD" },
	{ "824CC7A0-36A8-11E3-890A-952519AD3F61",       "OpenBSD Data",       "OpenBSD" },
	{ "49F48D32-B10E-11DC-B99B-0019D1879648",       "NetBSD FFS",         "NetBSD" },
	{ "48465300-0000-11AA-AA11-00306543ECAC",       "Apple HFS+",         "macOS" },
	{ "7C3457EF-0000-11AA-AA11-00306543ECAC",       "Apple APFS",         "macOS" },
	{ "53746F72-6167-11AA-AA11-00306543ECAC",       "Apple CoreStorage",  "macOS" },
	{ "6A898CC3-1DD2-11B2-99A6-080020736631",       "Solaris ZFS",        "Solaris" },
	{ "FE3A2A5D-4F32-41A7-B725-ACCC3285A309",       "ChromeOS Kernel",    "ChromeOS" },
	{ "3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC",       "ChromeOS Rootfs",    "ChromeOS" },
	{ "AA31E02A-400F-11DB-9590-000C2911D1B8",       "VMware VMFS",        "VMware" },
	{ "42465331-3BA3-10F1-802A-4861696B7521",       "Haiku BeFS",         "Haiku" },
	{ "CEF5A9AD-73BC-4601-89F3-CDEEEEE321A1",       "QNX PowerSafe",      "QNX 6.x" },
	{ "1B81E7E6-F50D-419B-A739-2AEEF8DA3335",       "Android User Data",  "Android" },
	{ "DC76DDA9-5AC1-491C-AF42-A82591580C0D",       "Android Data",       "Android" },
};

/**
 * @brief Known MBR system IDs of interest
 */
static const mbr_id_map_t mbr_id_map[] = {
	{ 0x01, "FAT12",        "DOS" },
	{ 0x02, "XENIX Root",   "XENIX" },
	{ 0x03, "XENIX Usr",    "XENIX" },
	{ 0x04, "FAT16 <32MB",  "DOS" },
	{ 0x06, "FAT16",        "DOS" },
	{ 0x07, "Windows/NTFS", "Windows" },
	{ 0x0B, "FAT32 CHS",    "DOS" },
	{ 0x0C, "FAT32 LBA",    "DOS" },
	{ 0x2E, "RedSea",       "TempleOS" },
	{ 0x39, "Plan 9",       "Plan 9" },
	{ 0x63, "MenuetOS",     "MenuetOS" },
	{ 0x76, "AROS",         "AROS" },
	{ 0x82, "Linux Swap",   NULL },
	{ 0x83, "Linux",        "Linux" },
	{ 0x87, "Windows HPFS", "Windows NT" },
	{ 0x8B, "Windows HPFS", "Windows NT" },
	{ 0x8C, "Windows HPFS", "Windows NT" },
	{ 0x8D, "Hidden FAT12", "DOS"},
	{ 0x8E, "Linux LVM",    "Linux" },
	{ 0x97, "Hidden FAT32", "DOS"},
	{ 0xA5, "FreeBSD",      "BSD" },
	{ 0xA6, "OpenBSD",      "BSD" },
	{ 0xA9, "NetBSD",       "BSD" },
	{ 0xAB, "Apple Boot",   "macOS" },
	{ 0xAC, "Apple RAID",   "macOS" },
	{ 0xAD, "ADFS FileCore","RiscOS" },
	{ 0xAF, "macOS HFS+",   "macOS" },
	{ 0xB1, "QNX PSFS",     "QNX 6.x" },
	{ 0xB2, "QNX PSFS",     "QNX 6.x" },
	{ 0xEB, "BeFS/Haiku",   "Haiku" },
	{ 0xFB, "VMware VMFS",  "VMware" },
};


/**
 * @brief Update a CRC32 with additional data (used by GPT)
 *
 * @param crc Current CRC32 value
 * @param data Pointer to input buffer
 * @param len Length of input buffer in bytes
 * @return Updated CRC32 value
 */
uint32_t crc32_update(uint32_t crc, const void *data, size_t len);

/**
 * @brief Copy a single file
 *
 * Loads the entire source into memory and writes it to the destination.
 *
 * @param source Source path
 * @param destination Destination path
 * @return true on success, false on failure
 */
bool copy_file(const char* source, const char* destination);

/**
 * @brief Recursively copy a directory tree
 *
 * @param source Source directory path
 * @param destination Destination directory path
 * @return true on success, false on failure
 */
bool copy_directory(const char* source, const char* destination);

/**
 * @brief Wait for a single numeric key press within a range
 *
 * @param min Minimum allowed digit
 * @param max Maximum allowed digit
 * @return The digit pressed
 */
uint8_t numeric_choice(uint8_t min, uint8_t max);

/**
 * @brief Print centred text in the current console
 *
 * @param format printf-style format string
 * @param ... Variadic arguments
 */
void centre_text(const char* format, ...) PRINTF_LIKE(1,2);

/**
 * @brief Print a centred white-on-red warning message
 *
 * @param warning_message Message to display
 */
void warning(const char* warning_message);

/**
 * @brief Align a 64-bit value up to the nearest multiple
 *
 * @param x Input value
 * @param a Alignment (power-of-two multiple recommended)
 * @return x aligned up to a
 */
uint64_t align_up_u64(uint64_t x, uint64_t a);

/**
 * @brief Encode an ASCII name as UTF-16LE (max 36 code units)
 *
 * @param ascii Input ASCII string
 * @param out36 Output UTF-16LE buffer of 36 elements
 */
void make_utf16le_name(const char *ascii, uint16_t out36[36]);

/**
 * @brief Generate a random version-4 GUID
 *
 * @param out16 Output buffer of 16 bytes
 */
void random_guid_v4(uint8_t out16[16]);

/**
 * @brief Write raw sectors to a block device
 *
 * @param devname Device name (e.g. "hd0")
 * @param start_lba Starting LBA
 * @param buf Buffer to write
 * @param bytes Number of bytes to write
 * @param sector_bytes Logical sector size in bytes
 * @return true on success, false on failure
 */
bool write_lbas(const char *devname, uint64_t start_lba, const void *buf, uint32_t bytes, uint32_t sector_bytes);

/**
 * @brief Flatten a device and install GPT + ESP + RetroFS
 *
 * Creates a GPT containing an EFI System Partition and a RetroFS partition,
 * writes a prebuilt FAT32 ESP image, and prepares the RetroFS partition.
 *
 * @param devname Storage device name (e.g. "hd0")
 * @param esp_image_vfs_path VFS path to the prebuilt FAT32 ESP image
 * @return true on success, false on failure
 */
bool install_gpt_esp_rfs_whole_image(const char *devname, const char *esp_image_vfs_path);

/**
 * @brief Copy userland from installation media to the device
 *
 * @param devname Target device name
 */
void copy_userland(const char* devname);

/**
 * @brief Locate and format the first GPT RetroFS partition
 *
 * @param dev Target storage device
 * @return true on success, false on failure
 */
bool prepare_rfs_partition(storage_device_t* dev);

/**
 * @brief Summarise existing content on a device for the installer
 *
 * Produces a short, user-facing description of detected partitions and a
 * heuristic "likely OS". Sets *usable to true if inspection completed
 * without I/O or parse errors.
 *
 * @param dev Target storage device
 * @param out Output message buffer
 * @param out_len Size of output buffer in bytes
 * @param usable Set to true if the device was readable and parsable
 * @return true if a summary was produced, false on low-level failure
 */
bool probe_device_summary(const storage_device_t *dev, char *out, size_t out_len, bool *usable);
