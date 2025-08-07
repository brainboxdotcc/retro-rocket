#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Creates a full file path by combining the current selected directory (CSD) with a relative path.
 *
 * If the given path is absolute (starts with '/'), it returns the path as is. If it is relative, it combines it
 * with the current selected directory (CSD).
 *
 * @param ctx BASIC interpreter context
 * @param relative The relative file path to be converted into an absolute path
 * @return The full file path as a string
 */
const char* make_full_path(struct basic_ctx* ctx, const char* relative);

/**
 * @brief Handles the OPENIN statement in BASIC.
 *
 * Since OPENIN is a function in Retro Rocket BASIC, this statement is not supported and raises an error.
 *
 * @param ctx BASIC interpreter context
 */
void openin_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the OPENUP statement in BASIC.
 *
 * Since OPENUP is a function in Retro Rocket BASIC, this statement is not supported and raises an error.
 *
 * @param ctx BASIC interpreter context
 */
void openup_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the OPENOUT statement in BASIC.
 *
 * Since OPENOUT is a function in Retro Rocket BASIC, this statement is not supported and raises an error.
 *
 * @param ctx BASIC interpreter context
 */
void openout_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the READ statement in BASIC.
 *
 * Since the READ statement is not supported in Retro Rocket BASIC (as it relies on DATA statements),
 * this function raises an error.
 *
 * @param ctx BASIC interpreter context
 */
void read_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the CLOSE statement in BASIC.
 *
 * This function closes the file associated with the file descriptor returned by the OPEN statement.
 *
 * @param ctx BASIC interpreter context
 */
void close_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the EOF function in BASIC.
 *
 * The EOF function checks whether the end of file has been reached. It is not supported in Retro Rocket BASIC.
 * This function raises an error.
 *
 * @param ctx BASIC interpreter context
 */
void eof_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the DELETE statement in BASIC.
 *
 * This function deletes the specified file from the file system.
 *
 * @param ctx BASIC interpreter context
 */
void delete_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the MKDIR statement in BASIC.
 *
 * This function creates a new directory at the specified path.
 *
 * @param ctx BASIC interpreter context
 */
void mkdir_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the MOUNT statement in BASIC.
 *
 * This function mounts a file system at the specified path, allowing access to the device.
 *
 * @param ctx BASIC interpreter context
 */
void mount_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the RMDIR statement in BASIC.
 *
 * This function removes a directory from the file system.
 *
 * @param ctx BASIC interpreter context
 */
void rmdir_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the WRITE statement in BASIC.
 *
 * This function writes data to the file specified by the file descriptor. The data is retrieved from the
 * printable syntax of the variable.
 *
 * @param ctx BASIC interpreter context
 */
void write_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the CHDIR statement in BASIC.
 *
 * This function changes the current selected directory (CSD) for the running process.
 *
 * @param ctx BASIC interpreter context
 */
void chdir_statement(struct basic_ctx* ctx);

/**
 * @brief Returns the type of a file or directory (either "file" or "directory").
 *
 * This function checks if the specified path corresponds to a directory or a regular file.
 *
 * @param ctx BASIC interpreter context
 * @return A string indicating whether the path is a "file" or "directory"
 */
char* basic_filetype(struct basic_ctx* ctx);

/**
 * @brief Handles the DATA statement in BASIC.
 *
 * The DATA statement is not supported in Retro Rocket BASIC. This function raises an error, suggesting
 * to use files instead.
 *
 * @param ctx BASIC interpreter context
 */
void data_statement(struct basic_ctx* ctx);

/**
 * @brief Handles the RESTORE statement in BASIC.
 *
 * Since DATA statements are not supported in Retro Rocket BASIC, this function raises an error indicating
 * there is nothing to restore.
 *
 * @param ctx BASIC interpreter context
 */
void restore_statement(struct basic_ctx* ctx);

/**
 * @brief Opens a file for reading.
 *
 * This function opens a file in read-only mode and returns the file descriptor.
 *
 * @param ctx BASIC interpreter context
 * @return The file descriptor for the opened file
 */
int64_t basic_openin(struct basic_ctx* ctx);

/**
 * @brief Opens a file for writing.
 *
 * This function opens a file in write-only mode and returns the file descriptor.
 *
 * @param ctx BASIC interpreter context
 * @return The file descriptor for the opened file
 */
int64_t basic_openout(struct basic_ctx* ctx);

/**
 * @brief Opens a file for both reading and writing.
 *
 * This function opens a file in read-write mode and returns the file descriptor.
 *
 * @param ctx BASIC interpreter context
 * @return The file descriptor for the opened file
 */
int64_t basic_openup(struct basic_ctx* ctx);

/**
 * @brief Checks if the end of file (EOF) is reached.
 *
 * This function checks whether the specified file descriptor has reached the end of the file.
 *
 * @param ctx BASIC interpreter context
 * @return Returns 1 if EOF is reached, 0 otherwise
 */
int64_t basic_eof(struct basic_ctx* ctx);

/**
 * @brief Reads a single byte from the file.
 *
 * This function reads a single byte from the file specified by the file descriptor.
 *
 * @param ctx BASIC interpreter context
 * @return The byte read from the file
 */
int64_t basic_read(struct basic_ctx* ctx);

/**
 * @brief Returns the number of items (files) in the directory.
 *
 * This function retrieves the number of items (files) in the specified directory.
 *
 * @param ctx BASIC interpreter context
 * @return The number of items in the directory
 */
int64_t basic_getnamecount(struct basic_ctx* ctx);

/**
 * @brief Gets the size of a file in a directory.
 *
 * This function returns the size of a file at the specified index in the directory.
 *
 * @param ctx BASIC interpreter context
 * @return The size of the file at the specified index
 */
int64_t basic_getsize(struct basic_ctx* ctx);

/**
 * @brief Returns the name of a file or directory at a specified index.
 *
 * This function returns the name of a file or directory at the specified index in the directory.
 *
 * @param ctx BASIC interpreter context
 * @return The name of the file or directory at the specified index
 */
char* basic_getname(struct basic_ctx* ctx);

/**
 * @brief Creates a RAM disk from a specified device.
 *
 * This function creates a RAM disk from the specified device and returns a string identifier.
 *
 * @param ctx BASIC interpreter context
 * @return The identifier of the created RAM disk
 */
char* basic_ramdisk_from_device(struct basic_ctx* ctx);

/**
 * @brief Creates a RAM disk with a specified size.
 *
 * This function creates a RAM disk with the specified number of blocks and block size, and returns a string identifier.
 *
 * @param ctx BASIC interpreter context
 * @return The identifier of the created RAM disk
 */
char* basic_ramdisk_from_size(struct basic_ctx* ctx);


