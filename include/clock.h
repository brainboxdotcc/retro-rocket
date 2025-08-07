/**
 * @file clock.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012–2025
 * @brief Interfaces for accessing, converting, and formatting real-time clock (RTC) and timezone values.
 *
 * This header defines types and functions for:
 * - Reading the hardware Real-Time Clock (RTC) via CMOS I/O ports
 * - Representing dates/times as POSIX `time_t` values and `datetime_t` components
 * - Loading and interpreting IANA tzdb time zone files (`TZif` format)
 * - Converting between UTC and local time with DST adjustments
 * - Generating formatted date/time strings for display
 */

#pragma once

#include <kernel.h>
#include <stdint.h>

/**
 * @brief POSIX-style time representation.
 *
 * Number of seconds since 00:00:00 UTC on 1 January 1970 (the epoch).
 * This implementation uses a signed 64-bit type to allow for historical
 * and far-future date handling beyond the 2038 limit.
 */
typedef int64_t time_t;

/**
 * @brief Result of a timezone lookup for a given timestamp.
 */
typedef struct {
	int32_t utc_offset;   ///< Offset from UTC in seconds (positive east of Greenwich)
	bool is_dst;          ///< True if Daylight Saving Time is in effect
	const char* abbrev;   ///< Timezone abbreviation (pointer into TZ string table)
} tz_offset_t;

/** @brief Magic number at the start of a tzfile (`TZif`). */
#define TZ_MAGIC "TZif"

/**
 * @brief On-disk header structure for an IANA tzdb file.
 *
 * Version 1 headers are 44 bytes. Version 2/3 use the same layout but are followed by
 * additional data blocks. All integers are stored in network byte order (big-endian).
 */
struct tz_header {
	char magic[4];    ///< File magic: "TZif"
	char version[1];  ///< Version: '2', '3', or '\0' (v1)
	char reserved[15];///< Reserved/padding
	uint32_t ttisgmtcnt; ///< Count of UTC/local indicators
	uint32_t ttisstdcnt; ///< Count of standard/wall indicators
	uint32_t leapcnt;    ///< Number of leap-second corrections
	uint32_t timecnt;    ///< Number of transition times
	uint32_t typecnt;    ///< Number of local time types
	uint32_t charcnt;    ///< Size of abbreviation string table
} __attribute__((packed));

/**
 * @brief CMOS I/O port addresses for RTC register access.
 */
enum cmos_port_t {
	CMOS_ADDR = 0x70,  ///< Port for selecting CMOS register index
	CMOS_DATA = 0x71,  ///< Port for reading/writing CMOS register data
};

/**
 * @brief Component-wise date/time structure.
 *
 * All fields are binary (not BCD). The `century` field is only populated if
 * the BIOS supports CMOS register 0x32.
 */
typedef struct datetime {
	uint8_t century;  ///< Century number (e.g., 20 for 20xx)
	uint8_t year;     ///< Year within century (0–99)
	uint8_t month;    ///< Month (1–12)
	uint8_t day;      ///< Day of the month (1–31)
	uint8_t hour;     ///< Hour of the day (0–23)
	uint8_t minute;   ///< Minute (0–59)
	uint8_t second;   ///< Second (0–59)
} datetime_t;

/**
 * @brief Convert a `time_t` timestamp to a `datetime_t` (UTC).
 *
 * @param timestamp Seconds since epoch (UTC).
 * @param dt        Output pointer for the decomposed date/time.
 */
void datetime_from_time_t(time_t timestamp, datetime_t* dt);

/**
 * @brief Get the UTC offset/DST state for a given time from a loaded tzfile buffer.
 *
 * @param tzdata    Pointer to tzfile in memory.
 * @param timestamp Seconds since epoch (UTC).
 * @return Offset from UTC in seconds (including DST if applicable).
 */
int32_t get_local_offset_from_buffer(const uint8_t *tzdata, time_t timestamp);

/**
 * @brief Convert a UTC `time_t` to local time using the loaded timezone.
 *
 * @param timestamp Seconds since epoch (UTC).
 * @return Local time as seconds since epoch.
 */
time_t local_time(time_t timestamp);

/**
 * @brief Load and parse a timezone file from the system tzdb.
 *
 * @param timezone IANA timezone name (e.g. "Europe/London").
 * @return True if successfully loaded, false if not found or invalid.
 */
bool load_timezone(const char* timezone);

/**
 * @brief Get the current date/time as a formatted string.
 *
 * The returned string uses the format:
 * `"Mon, 28 Jul 2025 01:45:12"`.
 *
 * @return Pointer to a static buffer (overwritten on each call).
 */
const char* get_datetime_str();

/**
 * @brief Read the current RTC value into a `datetime_t`.
 *
 * @param dt Output pointer for current date/time (UTC).
 */
void get_datetime(datetime_t* dt);

/**
 * @brief Initialise the Real-Time Clock (RTC) subsystem.
 *
 * Configures RTC access and disables periodic interrupts.
 * Must be called during early kernel initialisation.
 */
void init_realtime_clock();

/**
 * @brief Get the current system time in seconds since epoch.
 *
 * @param t Optional pointer to store the result.
 * @return Seconds since 00:00:00 UTC on 1 Jan 1970.
 */
time_t time(time_t* t);

/**
 * @brief Calculate day-of-year (1–366) for a given date.
 *
 * @param year  Full year (e.g. 2025).
 * @param month Month number (1–12).
 * @param day   Day of the month (1–31).
 * @return Ordinal day of the year (1–366).
 */
uint64_t day_of_year(uint64_t year, uint8_t month, uint8_t day);

/**
 * @brief Get the weekday index for a given date.
 *
 * 0 = Sunday, 1 = Monday, ... 6 = Saturday.
 *
 * @param dt Pointer to date structure.
 * @return Weekday index (0–6).
 */
int get_weekday_from_date(datetime_t* dt);
