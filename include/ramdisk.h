/**
 * @file ramdisk.h
 * @author Craig Edwards
 * @copyright Copyright (c) 2012–2025.
 */

#pragma once

#include "kernel.h"

/**
 * @brief Create a new, empty RAM-backed block device
 *
 * @param blocks     Number of logical blocks to allocate
 * @param blocksize  Size of each logical block in bytes
 *
 * @return
 *   On success, returns the immutable device name (e.g. "ram0")
 *   On failure, returns `NULL` and sets an FS error (usually
 *   `FS_ERR_OUT_OF_MEMORY`).
 *
 * @note Memory for the device backing store is allocated and zero-initialised
 */
const char* init_ramdisk(size_t blocks, size_t blocksize);

/**
 * @brief Create a RAM-backed device by copying an existing storage device
 *
 * @param storage  Name of the source storage device to clone (e.g. "hd0")
 *
 * @return
 *   On success, returns the new RAM device name (e.g. "ram1")
 *   On failure, returns `NULL` and sets an FS error
 *   (`FS_ERR_NO_SUCH_DEVICE`, `FS_ERR_OUT_OF_MEMORY`, or I/O errors)
 *
 * @details
 *   The destination device is sized to match the source geometry and the
 *   entire contents are copied in block-sized chunks
 */
const char* init_ramdisk_from_storage(const char* storage);

/**
 * @brief Wrap an existing memory buffer as a RAM-backed block device
 *
 * @param memory     Pointer to caller-owned, suitably aligned buffer containing
 *                   the device contents
 * @param blocks     Number of logical blocks present in @p memory
 * @param blocksize  Size of each logical block in bytes
 *
 * @return
 *   On success, returns the device name (e.g. "ram2")
 *   On failure, returns `NULL` and sets an FS error
 *
 * @warning The ramdisk layer does **not** copy @p memory; the caller must
 *          ensure the buffer remains valid for the lifetime of the device
 */
const char* init_ramdisk_from_memory(uint8_t* memory, size_t blocks, size_t blocksize);

/**
 * @brief Inflate and mount the initial ramdisk supplied by the bootloader
 *
 * @param compressed_image  Pointer to a gzip stream (boot module)
 * @param compressed_size   Size of the gzip stream in bytes
 *
 * @return
 *   `true` if the image was successfully decompressed, registered as a ramdisk,
 *   and mounted as root; otherwise this function calls `preboot_fail()` and
 *   does not return
 *
 * @details
 *   The gzip stream is decompressed into freshly allocated RAM. The result is
 *   exposed as a ramdisk with a logical block size of 2048 bytes and mounted
 *   read-only as ISO-9660. This path is used by live USB boot.
 */
bool mount_initial_ramdisk(uint8_t* compressed_image, size_t compressed_size);
